# learned_imv_thread_hashing
Exploring the learned models along with interleaved multi-vectorization to improve the performance of hashing. In this project, some code blocks are re-used from db-imv repo (https://github.com/fzhedu/db-imv). 


Running steps from the project root:
1) cd learned_imv_threaded_hashing/build/release
2) Generate the approperiate config files as follows:

sh ../../scripts/evaluation/base_configs_maker.sh -RUN_LEARNED_TECHNIQUES 1 -RUN_LEARNED_TECHNIQUES_WITH_FIRST_LEVEL_ONLY 1 -NUM_THREADS_FOR_EVALUATION 64 -LS_FOR_SORT_MERGE_DEFAULT_FANOUT 2097152 -LS_FOR_SORT_MERGE_DEFAULT_ARCH_SECOND_LEVEL 2097152  -LS_FOR_SORT_MERGE_DEFAULT_SAMPLING_RATE 0.01 -LOAD_RELATIONS_FOR_EVALUATION 1 -RELATION_R_PATH '"'/spinning/sabek/learned_join_datasets/r_UNIQUE_v3_uint32_uint32_128000000.txt'"' -RELATION_R_NUM_TUPLES 128E6

sh ../../scripts/evaluation/eth_configs_maker.sh   -BUCKET_SIZE 1 \
                                                   -PREFETCH_DISTANCE 128 \
                                                   -USE_MURMUR3_HASH 1 \
                                                   -NPJ_ETH_AVX_IMV 1 \
                                                   -NPJ_SIMDStateSize 5

3) cmake -DCMAKE_BUILD_TYPE=Release -DVECTORWISE_BRANCHING=on  ../..

4) make

5) ./hashing_runner

Notes:
- Currently, we support 8-bytes tuples (4-bytes key, 4-bytes payload)
- BUCKET_SIZE must be 1
- Set RUN_LEARNED_TECHNIQUES with 0, if you want to run non-learned variations (both naive and vectorized)
- Set LOAD_RELATIONS_FOR_EVALUATION 0, if you want to generate the dataset on-the-fly
- Set LS_FOR_SORT_MERGE_DEFAULT_FANOUT, LS_FOR_SORT_MERGE_DEFAULT_ARCH_SECOND_LEVEL and LS_FOR_SORT_MERGE_DEFAULT_SAMPLING_RATE to control the RMI model parameters
