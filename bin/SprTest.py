#!/home/hx/venv/bin/python3.10
import asyncio
import time
import os

async def run_executable(executable_path: str) -> float:
    """
    启动一个可执行文件并等待其完成（协程方式）
    :param executable_path: 可执行文件路径（如 "C:/app.exe"）
    """
    # 创建子进程
    # asyncio.create_subprocess_exec() 异步创建子进程执行指定程序

    process = await asyncio.create_subprocess_exec(
        executable_path,
        # stdout=asyncio.subprocess.PIPE, 重定向子进程的标准输出到内存缓冲区
        # stderr=asyncio.subprocess.PIPE  重定向标准错误输出
        stdout=asyncio.subprocess.DEVNULL,   # 关闭标准输出记录
        stderr=asyncio.subprocess.DEVNULL,   # 关闭错误输出记录
        stdin=asyncio.subprocess.DEVNULL,    # 关闭未使用的输入管道
        close_fds=True,                      # 关闭继承的文件描述符
        start_new_session=True,              # 脱离终端控制
    )

    # 等待子进程完成并捕获输出
    start = time.perf_counter()
    await process.wait()                     # 快速执行短进程
    end = time.perf_counter() - start

    return end

async def main(nums = 100):
    # 定义要启动的可执行文件路径（示例中启动3次）
    exe_path = "/home/hx/rpc_reactor/bin/test_rpc_client"
    tasks = []

    for _ in range(nums):
        tasks.append(run_executable(exe_path))

    # 并发运行所有协程
    # await会挂起协程，直到其后的可等待对象完成
    # asyncio.gather() 并发执行多个异步任务，并统一收集所有任务的返回结果（或异常）
    results = await asyncio.gather(*tasks) 

    total = 0.0

    for i in results:
        total += i;
    # 输出结果
    print('average = ',total)

if __name__ == "__main__":
    asyncio.run(main())