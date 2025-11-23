#!/bin/bash

HOST="127.0.0.1"
PORT=8080
DURATION=300
THREADS_LIST=(1 4 8 16 32 64) 
OUTPUT_FILE="benchmark_results2.csv"

echo "Workload,Threads,Throughput,Avg_Latency,P99_Latency,Server_CPU_Util,DB_Disk_Util" > $OUTPUT_FILE

run_test() {
    WORKLOAD_TYPE=$1
    WORKLOAD_NAME=$2
    
    echo "=== Starting $WORKLOAD_NAME Benchmark ==="
    
    for THREADS in "${THREADS_LIST[@]}"; do
        echo "Running: $WORKLOAD_NAME | Threads: $THREADS | Duration: $DURATION s"
        
        taskset -c 0-3 ./kv_server $PORT 500 > server.log 2>&1 &
        SERVER_PID=$!
        sleep 2
        
        mpstat -P 0-3 1 $DURATION > cpu_stats.txt &
        iostat -dx 1 $DURATION > disk_stats.txt &
        taskset -c 4-7 ./loadgen $HOST $PORT $THREADS $DURATION $WORKLOAD_TYPE > loadgen_output.txt
        
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null
        
        THROUGHPUT=$(grep "Throughput:" loadgen_output.txt | awk '{print $2}')
        
        # Extract Latency
        AVG_LAT=$(grep "Avg Latency:" loadgen_output.txt | awk '{print $3}')
        P99_LAT=$(grep "P99 Latency:" loadgen_output.txt | awk '{print $3}')
        
        CPU_IDLE=$(tail -n 1 cpu_stats.txt | awk '{print $12}')
        CPU_UTIL=$(echo "100 - $CPU_IDLE" | bc)
        
        DISK_UTIL=$(grep -m 1 "nvme0n1" disk_stats.txt | awk '{print $NF}') 
        if [ -z "$DISK_UTIL" ]; then DISK_UTIL="0"; fi

        echo "$WORKLOAD_NAME,$THREADS,$THROUGHPUT,$AVG_LAT,$P99_LAT,$CPU_UTIL,$DISK_UTIL" >> $OUTPUT_FILE
        
        echo "Finished Threads: $THREADS. Result: $THROUGHPUT req/s"
        sleep 2
    done
}
run_test 0 "Read_Heavy"
run_test 1 "Write_Heavy"
run_test 2 "Balanced" 

echo "Benchmark Complete. Results saved to $OUTPUT_FILE"
