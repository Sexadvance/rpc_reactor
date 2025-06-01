#!/bin/bash

# 获取毫秒级时间戳
current_millis() {
    echo $(date +%s%3N)
}

# 格式化毫秒时间戳为可读日期
format_millis() {
    local millis=$1
    local seconds=$((millis / 1000))
    local milliseconds=$((millis % 1000))
    local date_str=$(date -d "@$seconds" "+%Y-%m-%d %H:%M:%S")
    printf "%s.%03d" "$date_str" "$milliseconds"
}

# 记录脚本开始执行的时间（毫秒精度）
start_millis=$(current_millis)
start_formatted=$(format_millis $start_millis)

echo "脚本开始执行时间: $start_formatted"
echo "启动5个test_rpc_client进程..."

# 并发启动10个客户端程序
for i in {1..10}; do
    # 每个程序输出到单独的日志文件
    ./test_rpc_client &> /dev/null &
    echo "  进程 $! 已启动 (输出到 client_$i.log)"
done

echo "等待所有进程完成..."
# 等待所有后台任务完成
wait

# 记录完成时间（毫秒精度）
end_millis=$(current_millis)
end_formatted=$(format_millis $end_millis)

# 计算执行持续时间（毫秒）
duration_ms=$((end_millis - start_millis))

# 将毫秒转换为小时、分钟、秒和毫秒
milliseconds=$((duration_ms % 1000))
total_seconds=$((duration_ms / 1000))
seconds=$((total_seconds % 60))
total_minutes=$((total_seconds / 60))
minutes=$((total_minutes % 60))
hours=$((total_minutes / 60))

printf "\n所有客户端进程已完成\n"
echo "程序执行完成时间: $end_formatted"
echo "脚本开始执行时间: $start_formatted"
echo "执行总耗时: ${hours}小时 ${minutes}分钟 ${seconds}秒 ${milliseconds}毫秒"
echo "脚本将在5秒后退出..."

# 倒计时退出
for i in {5..1}; do
    echo -n "$i "
    sleep 0.999 # 更精确的睡眠时间
done
echo " 0"

# 获取脚本退出时间（毫秒精度）
exit_millis=$(current_millis)
exit_formatted=$(format_millis $exit_millis)
echo "脚本退出时间: $exit_formatted"