/*
 * Function: Introduce particles into cells meeting transient physical conditions
 * 
 * This UDF randomly injects a fixed number of particles (P_NUM) at every SPAN time steps. 
 * Particles are placed at the centroids of cells that satisfy a user-defined condition (In this example code: y > 0).
 * 
 * It is designed for scenarios where nucleation sites appear dynamically (e.g., cavitation, crystal precipitation). The number of excited free nuclei in a real physical scenario is also definitely subject to transient changes. Please modify the corresponding variables (N_pick) according to your needs.
 * The injection source in Fluent (e.g., group/cone) must have at least P_NUM particles available. So you can set a huge number to ensure no mistake.
 * 
 * Algorithm:
 * 1. At specified time steps, collect all cell centroids meeting the condition.
 * 2. Gather these coordinates from all parallel processes to a master (node 0). This aggregation algorithm makes use of the author's open-source code from the other end.
 * 3. Master randomly selects P_NUM positions (with replacement) from the global pool.
 * 4. Broadcast the selected positions to all processes via a global sum (which here works as a broadcast).
 * 5. Reposition the first P_NUM particles in the injection source to these coordinates; remove the rest.
 * 
 * Note: The condition "center[1] > 0" is hardcoded – adjust for your application.
 */

#include "udf.h"

/* ==================== User-adjustable parameters ==================== */

#define INIT_DATA_SIZE 128    /**< Initial capacity of local coordinate arrays. The arrays grow automatically if needed. */
#define SPAN 1                /**< Injection interval: perform injection every SPAN time steps. */
#define P_NUM 10              /**< Number of particles to inject per injection step. */

/* ==================================================================== */

DEFINE_DPM_INJECTION_INIT(Introducing_particles, I)
{
    /* Only act at the specified time steps */
    if (RP_Get_Integer("time-step") % SPAN == 0)
    {
        Domain* domain = Get_Domain(1);                 /* Get fluid domain */
        Thread* t = Lookup_Thread(domain, 2);           /* Thread ID for cell loop – must match your mesh! */
        cell_t c;
        real center[ND_ND];                             /* Cell centroid coordinates */

        /* --- Step 1: Collect candidate coordinates (cells where y>0) on each process --- */
        int capacity = INIT_DATA_SIZE;
        real* x = (real*)malloc(capacity * sizeof(real));
        real* y = (real*)malloc(capacity * sizeof(real));
        int idx = 0;                                    /* Number of cells found locally */

        begin_c_loop_int(c, t)                          /* Loop over all cells in the thread */
        {
            C_CENTROID(center, c, t);                   /* Get centroid of cell c */
            if (center[1] > 0)                          /* Condition: y-coordinate > 0 */
            {
                if (idx >= capacity) {                  /* Resize arrays if full */
                    capacity *= 2;
                    x = (real*)realloc(x, capacity * sizeof(real));
                    y = (real*)realloc(y, capacity * sizeof(real));
                }
                x[idx] = center[0];                     /* Store x-coordinate */
                y[idx] = center[1];                     /* Store y-coordinate */
                idx++;
            }
        }
        end_c_loop(c, t);

        /* Global sum to know total number of candidate cells across all processes */
        int total_valid = PRF_GISUM1(idx);

        /* If no candidate cells exist anywhere, remove all particles */
        if (total_valid == 0)
        {
            Particle* p;
            loop(p, I->p_init) {
                MARK_PARTICLE(p, P_FL_REMOVED);          /* Mark for removal */
            }
        }
        else
        {
            /* --- Step 2: Gather all candidate coordinates to process 0 --- */
            real* global_x = NULL;
            real* global_y = NULL;

            if (myid == 0)                               /* Master process */
            {
                global_x = (real*)malloc(total_valid * sizeof(real));
                global_y = (real*)malloc(total_valid * sizeof(real));

                /* Copy local data first */
                for (int i = 0; i < idx; i++) {
                    global_x[i] = x[i];
                    global_y[i] = y[i];
                }
                int offset = idx;

                /* Receive data from each other process in order */
                for (int node = 1; node < compute_node_count; node++)
                {
                    int token = node;
                    PRF_CSEND_INT(node, &token, 1, 0);      /* Send token to indicate readiness */
                    int recv_num;
                    PRF_CRECV_INT(node, &recv_num, 1, node); /* Receive number of cells from that node */
                    PRF_CRECV_REAL(node, &global_x[offset], recv_num, node);
                    PRF_CRECV_REAL(node, &global_y[offset], recv_num, node);
                    offset += recv_num;
                }
            }
            else                                           /* Worker processes */
            {
                int token;
                PRF_CRECV_INT(0, &token, 1, 0);             /* Wait for token from master */
                PRF_CSEND_INT(0, &idx, 1, myid);            /* Send number of local cells */
                PRF_CSEND_REAL(0, x, idx, myid);            /* Send local x array */
                PRF_CSEND_REAL(0, y, idx, myid);            /* Send local y array */
            }

            /* --- Step 3: Master selects P_NUM random positions (with possible repetition) --- */
            int N_pick = P_NUM;
            if (N_pick > total_valid) N_pick = total_valid;  /* Ensure we don't exceed available cells */

            real* pick_x = (real*)malloc(N_pick * sizeof(real));
            real* pick_y = (real*)malloc(N_pick * sizeof(real));
            real* iwork = (real*)malloc(N_pick * sizeof(real));  /* Work array for PRF_GRSUM */

            if (myid == 0)
            {
                for (int i = 0; i < N_pick; i++) {
                    int rand_idx = (int)(uniform_random() * total_valid);  /* Random index in [0, total_valid-1] */
                    pick_x[i] = global_x[rand_idx];
                    pick_y[i] = global_y[rand_idx];
                }
                free(global_x);
                free(global_y);
            }
            else {
                /* Initialize with zeros; will be overwritten by the broadcast via PRF_GRSUM */
                memset(pick_x, 0, N_pick * sizeof(real));
                memset(pick_y, 0, N_pick * sizeof(real));
            }

            /* Broadcast the selected positions to all processes using a global sum.
             * Since only master has non-zero values, the sum across all processes yields the same result on every node.
             * This trick avoids explicit point-to-point broadcast.
             */
            PRF_GRSUM(pick_x, N_pick, iwork);
            PRF_GRSUM(pick_y, N_pick, iwork);
            free(iwork);

            /* --- Step 4: Reposition the first N_pick particles; remove the rest --- */
            Particle* p;
            int i = 0;
            loop(p, I->p_init)                             /* Loop over all particles in the injection source */
            {
                if (i < N_pick) {
                    P_POS(p)[0] = pick_x[i];               /* Set x-position */
                    P_POS(p)[1] = pick_y[i];               /* Set y-position */
                    i++;
                }
                else {
                    MARK_PARTICLE(p, P_FL_REMOVED);         /* Remove extra particles */
                }
            }
            free(pick_x);
            free(pick_y);
        }
        free(x);
        free(y);
    }
    else
    {
        /* Not an injection step: remove all particles */
        Particle* p;
        loop(p, I->p_init)
        {
            MARK_PARTICLE(p, P_FL_REMOVED);
        }
    }
}
