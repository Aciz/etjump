/*
 * MIT License
 *
 * Copyright (c) 2025 ETJump team <zero@etjump.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <thread>
#include "etj_synchronization_context.h"

void ETJump::SynchronizationContext::startWorkerThreads(unsigned numThreads) {
  if (_running) {
    throw std::runtime_error("Worker threads are already running. Stop them "
                             "before starting them again.");
  }

  _running = true;

  for (int i = 0; i < numThreads; ++i) {
    auto t = std::thread([this] { worker(); });

    _threads.push_back(std::move(t));
  }
}

void ETJump::SynchronizationContext::stopWorkerThreads() {
  _running = false;

  _workAvailable.notify_all();

  for (auto &t : _threads) {
    t.join();
  }
}

void ETJump::SynchronizationContext::postTask(TaskFn task, CallbackFn callback,
                                              ErrorFn errorCallback) {
  std::lock_guard<std::mutex> lock(_incompletedMutex);

  _incompleted.push(std::make_unique<Operation>(
      std::move(task), std::move(callback), std::move(errorCallback)));

  _workAvailable.notify_one();
}

void ETJump::SynchronizationContext::processCompletedTasks() {
  std::lock_guard<std::mutex> lock(_completedMutex);

  while (!_completed.empty()) {
    auto op = std::move(_completed.front());
    _completed.pop();

    try {
      if (op->status == Operation::Status::Complete) {
        op->callback(std::move(op->result.value()));
      } else {
        op->errorCallback(std::move(op->error.value()));
      }
    } catch (const std::runtime_error &e) {
      op->errorCallback(e);
    }
  }
}

void ETJump::SynchronizationContext::worker() {
  while (_running) {
    std::unique_lock<std::mutex> incompletedLock(_incompletedMutex);
    _workAvailable.wait(incompletedLock, [this]() {
      return !_incompleted.empty() || !_running;
    });

    if (!_running) {
      // NOTE: we're currently dropping anything that wasn't processed to
      // technically there's a chance that we lose some information. Should
      // we instead loop through the existing incomplete list and execute
      // them?
      return;
    }

    auto operation = std::move(_incompleted.front());
    _incompleted.pop();
    incompletedLock.unlock();

    try {
      auto result = operation->task();

      operation->result = opt<std::unique_ptr<ResultBase>>(std::move(result));
      operation->status = Operation::Status::Complete;
    } catch (const std::runtime_error &e) {
      operation->error = opt<std::runtime_error>(e);
      operation->status = Operation::Status::Error;
    }

    std::lock_guard<std::mutex> completedLock(_completedMutex);
    _completed.push(std::move(operation));
  }
}
