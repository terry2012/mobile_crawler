#! /bin/sh

echo "Starting the process ..."
./bin/crawler &> output.log &

sleep 5

pid=$!;
echo "The process id is $pid."
while true;
do
        # got it from http://unix.stackexchange.com/questions/554/how-to-monitor-cpu-memory-usage-of-a-single-process
        wait_time=1
        delay=0.1
        memory_usage=`top -d $delay -b -n $wait_time -p $pid \
                |grep $pid \
                |sed -r -e "s;\s\s*; ;g" -e "s;^ *;;" \
                |cut -d' ' -f10 \
                |tr '\n' '+' \
                |sed -r -e "s;(.*)[+]$;\1;"`;
        memory_usage=`echo "$memory_usage" | xargs`
        if [ -z $memory_usage ]
        then
                echo "The process was done."
                exit 0
        fi

        echo $memory_usage
        result=`echo "$memory_usage > 80" | bc`
        if [[ $result -ge 1 ]]
        then
                kill -15 $pid
                echo "Terminating the process.."
                exit 0
        fi

        sleep 5
done
