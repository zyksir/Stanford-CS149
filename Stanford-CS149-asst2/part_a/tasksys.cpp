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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads), _num_threads(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    _thread_pool = new std::thread[_num_threads];
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    delete[] _thread_pool;
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::mutex mutex;
    int* nxt_task = new int(0);
    auto fetchRun = [runnable, num_total_tasks, &mutex, nxt_task]() {
        int cur_task = -1;
        while(cur_task < num_total_tasks) {
            {
                std::lock_guard<std::mutex> myLock(mutex);
                cur_task = *nxt_task;
                (*nxt_task)++;
            }
            if (cur_task >= num_total_tasks) {
                return;
            }
            runnable->runTask(cur_task, num_total_tasks);
        }
    };
    for (int i = 0; i < _num_threads; i++) {
        _thread_pool[i] = std::thread(fetchRun);
    }
    for (int i = 0; i < _num_threads; i++) {
        _thread_pool[i].join();
    }
    delete nxt_task;

}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelSpawn::sync() {
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads), _num_threads(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    _thread_pool = new std::thread[_num_threads];
    for(int i = 0; i < _num_threads; ++i) {
        _thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::fetchAndRun, this);
    }
}

void TaskSystemParallelThreadPoolSpinning::fetchAndRun() {
    int cur_task;
    int num_total_tasks;
    while(!_killed) {
        {
            std::lock_guard<std::mutex> myLock(_mutex);
            if (_all_finished || _nxt_task >= _num_total_tasks) continue;
            cur_task = _nxt_task++;
            num_total_tasks = _num_total_tasks;
        }
        _runnable->runTask(cur_task, num_total_tasks);
        {
            std::lock_guard<std::mutex> myLock(_mutex);
            _num_finished_tasks++;
            if (_num_finished_tasks == _num_total_tasks) {
                _all_finished = true;
                _finish_cond.notify_all();
            }
        }

    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    _killed = true;
    for(int i = 0; i < _num_threads; ++i) {
        _thread_pool[i].join();
    }
    delete[] _thread_pool;
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    {
        std::lock_guard<std::mutex> myLock(_mutex);
        _nxt_task = 0;
        _num_total_tasks = num_total_tasks;
        _num_finished_tasks = 0;
        _runnable = runnable;
        _all_finished = false;
    }

    std::unique_lock<std::mutex> lck(_mutex); 
    _finish_cond.wait(lck, [this]{ return this->_all_finished; }); 
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
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
    int cur_task = 0;
    int num_total_tasks = 0;
    while(!_killed) {
        {
            std::lock_guard<std::mutex> myLock(_mutex);
            cur_task = _nxt_task;
            num_total_tasks = _num_total_tasks;
            if (_nxt_task < _num_total_tasks) ++_nxt_task;
        }
        if (cur_task < num_total_tasks) {
            _runnable->runTask(cur_task, num_total_tasks);
            std::lock_guard<std::mutex> myLock(_mutex);
            _num_finished_tasks++;
            if (_num_finished_tasks == _num_total_tasks) {
                _all_finished = true;
                _finish_cond.notify_all();
            }
        } else {
            std::unique_lock<std::mutex> lck(_mutex);
            _wakeup_cond.wait(lck, [this]{ return _killed || _nxt_task < _num_total_tasks; }); 
            // numThreadRun++;
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
    // for (int i = 0; i < num_total_tasks; i++) {
    //     runnable->runTask(i, num_total_tasks);
    // }
    {
        std::lock_guard<std::mutex> myLock(_mutex);
        _nxt_task = 0;
        _num_total_tasks = num_total_tasks;
        _num_finished_tasks = 0;
        _runnable = runnable;
        _all_finished = false;
        // _wakeup_cond.notify_all();
    }

    _wakeup_cond.notify_all();

    {
        std::unique_lock<std::mutex> lck(_mutex); 
        _finish_cond.wait(lck, [this]{ return this->_all_finished; }); 
    }

    // numRun++;
    // std::cout << numRun << "\t" << numThreadRun << std::endl;
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
