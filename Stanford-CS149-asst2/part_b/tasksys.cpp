#include "tasksys.h"
#include <iostream>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads), _num_threads(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    _thread_pool = new std::thread[_num_threads];
    for(int i = 0; i < _num_threads; ++i) {
        _thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::fetchAndRun, this);
    }
}

void TaskSystemParallelThreadPoolSleeping::fetchAndRun() {
    int cur_task;
    int num_total_tasks;
    IRunnable* runnable = nullptr;
    TaskID cur_task_id;
    while(!_killed) {
        // TaskArgs cur_task;
        bool run_task = false;
        {
            std::lock_guard<std::mutex> myLock(_mutex);
            // std::cout << "size" << _ready_queue.size() << "," << _waiting_queue.size() << "," << _running_task.size() << std::endl;
            if (!_ready_queue.empty()) {
                // cur_task = _ready_queue.front();
                // std::cout << "top of ready_queue: " << task.task_id << "\t" << task.nxt_task << "/" << task.num_total_tasks << std::endl;
                auto& task = _ready_queue.front();
                cur_task = task.nxt_task; 
                num_total_tasks = task.num_total_tasks;
                runnable = task.runnable;
                cur_task_id = task.task_id;

                if (cur_task >= num_total_tasks) {
                    _ready_queue.pop();
                } else {
                    _ready_queue.front().nxt_task++;
                    run_task = true;
                }
            }
        }
        if (run_task) {
            runnable->runTask(cur_task, num_total_tasks);
            {
                std::lock_guard<std::mutex> myLock(_mutex);
                auto cur_it = _running_task.find(cur_task_id);
                if (cur_it == _running_task.end()) {
                    std::cout << "ERROR:\t" << cur_task_id << " not found" << std::endl;
                    continue;
                }
                cur_it->second++;
                if (cur_it->second == num_total_tasks) {
                    _running_task.erase(cur_it);
                    if (_running_task.empty()) {
                        _finish_cond.notify_all();
                        continue;
                    }
                    while(!_waiting_queue.empty()) {
                        auto& task = _waiting_queue.front();
                        bool satisfy = task.depend_tasks.empty() || std::all_of(
                            task.depend_tasks.begin(), task.depend_tasks.end(),
                            [this](const TaskID& t) { return this->_running_task.find(t) == this->_running_task.end(); }
                        );
                        if (!satisfy) break;
                        _ready_queue.push(task);
                        _wakeup_cond.notify_all();
                        _waiting_queue.pop();
                        // std::cout << "task ready! " << task.task_id << "," << task.depend_tasks.size() << "," << _max_finish_id << "/" << task.num_total_tasks << std::endl;
                    }
                }
            }
        } else {
            std::unique_lock<std::mutex> lck(_mutex);
            _wakeup_cond.wait(lck, [this]{ return _killed || !this->_ready_queue.empty(); }); 
        }
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    _killed = true;
    _wakeup_cond.notify_all();
    for(int i = 0; i < _num_threads; ++i) {
        _thread_pool[i].join();
    }
    delete[] _thread_pool;
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::vector<TaskID> noDeps;
    runAsyncWithDeps(runnable, num_total_tasks, noDeps);
    sync();
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //
    TaskID task_id = _nxt_task_id;
    ++_nxt_task_id;
    TaskArgs task{task_id, runnable, num_total_tasks, deps};
    {
        std::lock_guard<std::mutex> myLock(_mutex);
        bool satisfy = task.depend_tasks.empty() || std::all_of(
            task.depend_tasks.begin(), task.depend_tasks.end(),
            [this](const TaskID& t) { return this->_running_task.find(t) == this->_running_task.end(); }
        );
        if (satisfy) {
            _ready_queue.push(task);
            _wakeup_cond.notify_all();
        } else {
            _waiting_queue.push(task);
        }
        _running_task.emplace(task.task_id, 0);
    }
    // std::cout << "task_id:\t" << task_id << "\t" << max_depend_id << std::endl;
    return task_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {
    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //
    std::unique_lock<std::mutex> lck(_mutex); 
    _finish_cond.wait(lck, [this]{ return this->_running_task.empty(); }); 
}