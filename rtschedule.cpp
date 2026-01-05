#include <iostream>
#include <vector>
#include <algorithm>
#include <queue>
#include <numeric>

using namespace std;

struct Task {
    int id;
    int period;      
    int wcet;        
    int remaining;   
    int deadline;    
    int next_release_time;
};

// ---------------------------------------------------------
// 比较器定义 (保持最小堆逻辑不变)
// ---------------------------------------------------------
// RMS: 周期越小，优先级越高 (Period 越小排在堆顶)
struct CompareRMS {
    const vector<Task>& tasks;
    CompareRMS(const vector<Task>& t) : tasks(t) {}
    bool operator()(int lhs, int rhs) {
        // 如果 lhs 的周期 > rhs 的周期，则 lhs 优先级更低 (返回true沉底)
        return tasks[lhs].period > tasks[rhs].period; 
    }
};

// EDF: 截止时间越早，优先级越高 (Deadline 越小排在堆顶)
struct CompareEDF {
    const vector<Task>& tasks;
    CompareEDF(const vector<Task>& t) : tasks(t) {}
    bool operator()(int lhs, int rhs) {
        return tasks[lhs].deadline > tasks[rhs].deadline;
    }
};

int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }
int lcm(int a, int b) { return (a * b) / gcd(a, b); }

// ---------------------------------------------------------
// 核心调度函数：显式维护 "Running" 和 "Ready"
// ---------------------------------------------------------
template <typename Comparator>
void runPreemptiveSimulation(vector<Task> tasks, bool isRMS) {
    int simTime = 1;
    for(const auto& t : tasks) simTime = lcm(simTime, t.period);
    
    // 就绪队列：只存放 "Ready" 状态的任务
    std::priority_queue<int, vector<int>, Comparator> readyQueue((Comparator(tasks)));
    
    // 当前运行任务：-1 表示 CPU 空闲
    int currentTaskIdx = -1;

    cout << "=== Running Preemptive " << (isRMS ? "RMS" : "EDF") << " Simulation ===" << endl;

    for (int t = 0; t < simTime; ++t) {
        bool preemptionOccurred = false;

        // 1. 任务到达处理 (Arrival)
        for (int i = 0; i < tasks.size(); ++i) {
            if (t == tasks[i].next_release_time) {
                // 重置任务状态
                tasks[i].remaining = tasks[i].wcet;
                tasks[i].deadline = t + tasks[i].period;
                tasks[i].next_release_time += tasks[i].period;
                
                // 新任务加入就绪队列
                readyQueue.push(i);
            }
        }

        // 2. 抢占检查 (Preemption Check)
        // 如果当前有任务在运行，且就绪队列不为空，检查是否需要抢占
        if (currentTaskIdx != -1 && !readyQueue.empty()) {
            int topReadyIdx = readyQueue.top();
            
            // TODO: 实现抢占判断逻辑
            // 比较 topReadyIdx (挑战者) 和 currentTaskIdx (守擂者) 的优先级
            // 注意：isRMS 为 true 时比较 period，为 false 时比较 deadline
            // 如果 挑战者 优于 守擂者：
            //    1. 将 currentTaskIdx 放回 readyQueue
            //    2. 将 currentTaskIdx 设为 -1 (或直接切换)
            //    3. 记录发生了抢占 (用于日志输出)
            
            bool shouldPreempt = false;
            if (isRMS) {
                // RMS: 周期越短优先级越高 [cite: 14, 15]
                // ---------------------------------------------------------
                // TODO 1: 补全 RMS 抢占条件
                // ---------------------------------------------------------
                if (tasks[topReadyIdx].period < tasks[currentTaskIdx].period) {
                    shouldPreempt = true;
                }
                // ---------------------------------------------------------
            } else {
                // EDF: 截止时间越早优先级越高 
                // ---------------------------------------------------------
                // TODO 2: 补全 EDF 抢占条件
                // ---------------------------------------------------------
                if (tasks[topReadyIdx].deadline < tasks[currentTaskIdx].deadline) {
                    shouldPreempt = true;
                }
                // ---------------------------------------------------------
            }

            if (shouldPreempt) {
                // TODO 3: 执行抢占动作 (上下文切换)
                // ---------------------------------------------------------
                // 1. 把当前被抢占的任务放回就绪队列
                readyQueue.push(currentTaskIdx);
                // 2. 将 CPU 分配给更高优先级的任务
                currentTaskIdx = topReadyIdx;
                // 3. 从就绪队列中移除该任务 (因为它变成了 Running)
                readyQueue.pop();
                // ---------------------------------------------------------
                preemptionOccurred = true;
            }
        }

        // 3. 调度决策 (如果CPU空闲，从队列取任务)
        if (currentTaskIdx == -1 && !readyQueue.empty()) {
            currentTaskIdx = readyQueue.top();
            readyQueue.pop();
        }

        // 4. 执行与输出
        if (currentTaskIdx != -1) {
            if (preemptionOccurred) {
                cout << "  [!] Preemption at Time " << t << ": Switching to Task " << tasks[currentTaskIdx].id << endl;
            }
            cout << "Time " << t << ": Task " << tasks[currentTaskIdx].id << " is running." << endl;
            
            tasks[currentTaskIdx].remaining--;

            // 检查截止时间错失
            if (t >= tasks[currentTaskIdx].deadline && tasks[currentTaskIdx].remaining > 0) {
                 cout << "  !! Deadline Missed by Task " << tasks[currentTaskIdx].id << endl;
            }

            // 任务完成
            if (tasks[currentTaskIdx].remaining == 0) {
                cout << "  [+] Task " << tasks[currentTaskIdx].id << " Completed." << endl;
                currentTaskIdx = -1; // 释放 CPU
            }
        } else {
            cout << "Time " << t << ": Idle" << endl;
        }
    }
    cout << endl;
}

int main() {
    // 示例任务集：使用课件 Page 4 的参数来观察 RMS 的抢占
    // T1=5, C1=3; T2=8, C2=3
    // 在 T=5 时，Task 1 到达。此时 Task 2 (周期8) 可能正在运行。
    // 由于 Task 1 周期(5) < Task 2 周期(8)，RMS 应触发抢占。
    vector<Task> taskSet = {
        {1, 5, 3, 0, 0, 0},
        {2, 8, 3, 0, 0, 0}
    };

    runPreemptiveSimulation<CompareRMS>(taskSet, true);
    
    // 重置并运行 EDF
    taskSet = { {1, 5, 3, 0, 0, 0}, {2, 8, 3, 0, 0, 0} };
    runPreemptiveSimulation<CompareEDF>(taskSet, false);
    
    return 0;
}