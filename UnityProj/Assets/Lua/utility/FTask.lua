---@class Task
local Task = FLua.Class("FTask")

---@class Task.TaskStatus
local TaskStatus <const> =
{
	created = "created",
	running = "running",
	waiting_for_children = "waiting_for_children",
	completed = "completed",
	canceled = "canceled",
}
---@class Task.TaskAsyncOp
local TaskAsyncOp <const> =
{
	End = "end",
	Continue = "continue",
	Function = "function",
	Stop = "stop"
}

---@alias TaskAsyncResult Task.TaskAsyncOp | fun(task:Task, resumeEntry:TaskResumeEntryFunction)
---@alias TaskActionFunction fun(task:Task, ...):TaskAsyncResult,any
---@alias TaskCancelFunction fun(task:Task):Task.TaskAsyncOp
---@alias TaskContinuationFunction fun(task:Task):void
---@alias TaskResumeEntryFunction fun(task:Task):void
do
	function Task:__constructor()
		---@type nil|TaskActionFunction
		self.m_action = nil
		---@type nil|TaskCancelFunction
		self.m_cancelCallback = nil
		---@type nil|TaskContinuationFunction|TaskContinuationFunction[]
		self.m_continuations = nil
		self.m_status = TaskStatus.created
		self.m_canceled = false
		---@type nil|Task
		self.m_parent = nil
		self.m_result = nil
		---@type nil|integer
		self.m_waitingChildCount = nil
		---@type nil|TaskResumeEntryFunction
		self.m_nextResumeEntry = nil
		---@type nil|Task[]
		self.m_cancelNotifyList = nil
	end
	
	function Task:start()
		if self.m_status ~= TaskStatus.created then
			error("can not start a task which is already started")
		end

		-- make a new closure each time when need a entry, to avoid entering for multiple times
		local function makeResumEntry ()
			local firstTime = true
			local function resumeEntry (...)
				if not firstTime then
					return
				end
				firstTime = false

				if self.m_canceled then
					self:finishTask(nil)
					return
				end

				-- execute a step
				local asyncOp, result = self.m_action(self, ...)
				while asyncOp == TaskAsyncOp.Continue do
					if self.m_canceled then
						self:finishTask(nil)
						return
					end
					asyncOp, result = self.m_action(self, ...)
				end

				if self.m_canceled then
					self:finishTask(nil)
					return
				end

				if asyncOp == TaskAsyncOp.End then
					self:finishTask(result)
				elseif type(asyncOp) == TaskAsyncOp.Function then
					asyncOp(self, makeResumEntry())
				else
					error(([[bad return value from action, should be "end" or async operation, got %s (%s) %s]]):
						format(tostring(asyncOp), type(asyncOp), debug.traceback()))
				end
			end
			return resumeEntry
		end

		self.m_status = TaskStatus.running
		makeResumEntry()()
	end

	---Starts the Task and attach to parent
	---The parent task does not finish until the self task finished
	---@param parent Task
	function Task:startAsChild(parent)
		self:startAsChildEx(parent, true)
	end
	
	---Starts the Task and attach to parent
	---The parent task does not finish until the self task finished
	---param cancelWithParent: if parent Task is canceled, cancel self Task
	---@param parent Task
	---@param cancelWithParent boolean
	function Task:startAsChildEx(parent, cancelWithParent)
		if self.m_parent then
			error("can not start a child task which is already started")
		end
		self.m_parent = parent
		parent:incWaitingChildCount()
		
		if cancelWithParent then
			parent:appendCancelNotify(self)
		end
		
		self:start()
	end

	---Create a async operation that executes a sub Task
	---param sub: the sub task to execute, the self Task will continue after the sub Task finished
	---@param sub Task
	---@return fun(task:Task, resumeEntry:TaskResumeEntryFunction)
	function Task:executeSub(sub)
		local function subResumeFunction (task, resumeEntry)
			sub.m_parent = self
			self:appendCancelNotify(sub)
			sub.m_nextResumeEntry = resumeEntry
			sub:start()
		end
		return subResumeFunction
	end

	---Create a async operation that executes a sub Task. If sub Task is canceled, cancel the self Task
	---param sub: the sub task to execute, the self Task will continue after the sub Task finished
	---@param sub Task
	---@return fun(task:Task, resumeEntry:TaskResumeEntryFunction)
	function Task:completeSub(sub)
		return self:completeSubEx(sub, function(task, tsub)
			return tsub:isCanceled()
		end)
	end

	---checkCancel: return true to cancel parent task, return false to continue parent task
	---@param sub Task
	---@param checkCancel fun(task:Task, subTask:Task):boolean
	---@return fun(task:Task, resumeEntry:TaskResumeEntryFunction)
	function Task:completeSubEx(sub, checkCancel)
		local function subResumeFunction (task, resumeEntry)
			sub.m_parent = self
			self:appendCancelNotify(sub)
			sub.m_nextResumeEntry = function ()
				if checkCancel(task, sub) then
					task:cancel()
				end
				resumeEntry()
			end
			sub:start()
		end
		return subResumeFunction
	end

	---Execute the continuation when the target Task completes (including canceled)
	---param continuationFunction: a function to run when the Task finished. The finished task will be passed as an argument
	---@param continuationFunction TaskContinuationFunction
	---@return Task
	function Task:continueWith(continuationFunction)
		if self.m_status == TaskStatus.canceled or self.m_status == TaskStatus.completed then
			continuationFunction(self)
		else
			self:appendContinuation(continuationFunction)
		end
		return self
	end

	---Execute the continuation when the target Task completes successfully
	---param continuationFunction: a function to run when the Task finished. The finished task will be passed as an argument
	---@param continuationFunction TaskContinuationFunction
	---@return Task
	function Task:completeWith(continuationFunction)
		self:continueWith(function (task)
			if not task.m_canceled then
				continuationFunction(task)
			end
		end)
		return self
	end

	---Execute the continuation when the target Task canceled
	---param continuationFunction: a function to run when the Task completes. The finished task will be passed as an argument
	---@param continuationFunction TaskContinuationFunction
	---@return Task
	function Task:cancelWith(continuationFunction)
		self:continueWith(function (task)
			if task.m_canceled then
				continuationFunction(task)
			end
		end)
		return self
	end

	---@return any
	function Task:getResult()
		return self.m_result
	end

	function Task:cancel()
		if self.m_canceled or self.m_status == TaskStatus.completed then
			return
		end
		
		self.m_canceled = true
		
		local notifyList = self.m_cancelNotifyList
		if notifyList then
			for i = #notifyList, 1, -1 do
				notifyList[i]:cancel()
			end
		end
		local cancelCallback = self.m_cancelCallback
		if cancelCallback then
			local ret = cancelCallback(self)
			--if task has not started, finish callback should not be invoked
			--also, if cancel before start, cancel should not change status
			if ret == TaskAsyncOp.Stop and self.m_status ~= TaskStatus.created then
				self:finishTask(nil)
			end
		end
	end
	---@return boolean
	function Task:isActive()
		return self.m_status == TaskStatus.running and not self.m_canceled
	end
	---@return boolean
	function Task:isCanceled()
		return not not self.m_canceled
	end
	---@return boolean
	function Task:isCompleted()
		return self.m_status == TaskStatus.completed
	end
	---@return Task.TaskStatus
	function Task:getStatus()
		return self.m_status
	end

	---@param task Task sub task
	---@return Task self
	function Task:appendCancelNotify(task)
		local cancelNotifyList = self.m_cancelNotifyList
		if not cancelNotifyList then
			cancelNotifyList = {}
			self.m_cancelNotifyList = cancelNotifyList
		end
		cancelNotifyList[#cancelNotifyList+1] = task
		return self
	end

	---@param result any
	function Task:finishTask(result)
		if self.m_status ~= TaskStatus.running then
			return
		end

		self.m_result = result

		-- wait for children
		if not self.m_waitingChildCount then
			self:completeTask()
		else
			self.m_status = TaskStatus.waiting_for_children
		end
	end

	function Task:completeTask()
		if self.m_canceled then
			self.m_status = TaskStatus.canceled
		else
			self.m_status = TaskStatus.completed
		end
		
		-- execute continuations
		local continuations = self.m_continuations
		if not continuations then
		elseif type(continuations) == "function" then	--single function
			continuations(self)
		else	--function array
			for i = 1, #continuations do
				continuations[i](self)
			end
		end
		
		--notify parent
		local parent = self.m_parent
		if parent then
			parent:onChildComplete(self)
		end
		
		-- enter next resumeEntry (sub Task)
		local nextResumeEntry = self.m_nextResumeEntry
		if nextResumeEntry then
			nextResumeEntry()
		end
	end
	
	---@private
	---@param child Task
	function Task:onChildComplete(child)
		if self:decWaitingChildCount() then
			self:completeTask()
		end
	end

	---@private
	---@param f TaskContinuationFunction
	function Task:appendContinuation(f)
		local current = self.m_continuations
		if not current then
			self.m_continuations = f
		elseif type(current) == "function" then
			self.m_continuations = { current, f }
		else
			current[#current+1] = f
		end
	end

	---@private
	function Task:incWaitingChildCount()
		local waitingChildCount = self.m_waitingChildCount
		if waitingChildCount then
			self.m_waitingChildCount = waitingChildCount + 1
		else
			self.m_waitingChildCount = 1
		end
	end

	---@private
	-- return: true means get to zero
	function Task:decWaitingChildCount()
		if not self.m_waitingChildCount then return false end
		local waitingChildCount = self.m_waitingChildCount
		if waitingChildCount == 1 then
			self.m_waitingChildCount = false
			return true
		else
			self.m_waitingChildCount = waitingChildCount - 1
			return false
		end
	end
end
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------
---@param value any
---@return string
local function rawtype(value)
	local tp = type(value)
	if tp == "table" and value.__classname and value.__vtbl then
		tp = value.__classname
	end
	return tp
end

---@param value any
---@param who string
---@param argIndex integer
---@param needType string
local function checkParamType (value, who, argIndex, needType)
	if type(needType) == "table" then
		for _, nt in ipairs(needType) do
			if rawtype(value) == nt then
				return
			end
		end
		assert(false,
			([[bad argument #%d to %s in 'Task' (%s expected, got %s)]]):format(argIndex, who,
				table.concat(needType, ","),
				rawtype(value))
		)
	else
		assert(rawtype(value) == needType,
			([[bad argument #%d to %s in 'Task' (%s expected, got %s)]]):format(argIndex, who, needType, rawtype(value)))
	end
end

---@class TaskHelper
local TaskHelper = {}
TaskHelper.TaskStatus = TaskStatus

---@param action TaskActionFunction
---@return Task
function TaskHelper.create(action)
	return TaskHelper.createEx(action, nil)
end

---@param action TaskActionFunction
---@param cancelCallback TaskCancelFunction?
---@return Task
function TaskHelper.createEx(action, cancelCallback)
	checkParamType(action, 'action', 1, "function")
	checkParamType(cancelCallback, 'cancelCallback', 2, {"function", "boolean", "nil"})
	local obj = Task()
	obj.m_action = action
	obj.m_cancelCallback = cancelCallback or false
	return obj
end

---@param actionCo TaskActionFunction
---@return Task
function TaskHelper.createCo(actionCo)
	return TaskHelper.createCoEx(actionCo, nil)
end

---@param actionCo TaskActionFunction
---@param cancelCallback TaskCancelFunction?
---@return Task
function TaskHelper.createCoEx(actionCo, cancelCallback)
	checkParamType(actionCo, 'actionCo', 1, "function")
	return TaskHelper.createEx(TaskHelper.wrapCoroutine(actionCo), cancelCallback)
end

---@param action TaskActionFunction
---@return Task
function TaskHelper.createSteps(action)
	return TaskHelper.createStepsEx(action, nil)
end

---@param action TaskActionFunction
---@param cancelCallback? TaskCancelFunction
---@return Task
function TaskHelper.createStepsEx(action, cancelCallback)
	checkParamType(action, 'action', 1, "function")
	local step = 0
	local function actionFunc (task, ...)
		step = step + 1
		return action(task, step, ...)
	end
	return TaskHelper.createEx(actionFunc, cancelCallback)
end

---@param action TaskActionFunction
---@return Task
function TaskHelper.createOneStep(action)
	return TaskHelper.createOneStepEx(action, nil)
end

---@param action TaskActionFunction
---@param cancelCallback TaskCancelFunction?
---@return Task
function TaskHelper.createOneStepEx(action, cancelCallback)
	checkParamType(action, 'action', 1, "function")
	local firstStep = true
	local function actionFunc (task, result)
		if firstStep then
			firstStep = false
			return action(task)
		else
			return TaskAsyncOp.End, result
		end
	end
	return TaskHelper.createEx(actionFunc, cancelCallback)
end
---@param f TaskActionFunction
---@return TaskActionFunction
function TaskHelper.wrapCoroutine(f)
	local coroutine = require "coroutine"

	local co = coroutine.create(f)
	local coresume = coroutine.resume
	local costatus = coroutine.status

	local function wrapper (task, ...)
		if costatus(co) ~= "dead" then
			local succ, ret1, ret2 = coresume(co, task, ...)
			if succ then
				return ret1, ret2
			else
				local err = ret1
				local info = debug.getinfo(f, "nS")
				error(("wrapped coroutine function has error: %s. %s:%d")
					:format(tostring(err), info.source, info.linedefined))
			end
		else
			local info = debug.getinfo(f, "S")
			error(("wrapped coroutine exit without either finish or cancel: %s:%d")
				:format(info.source, info.linedefined))
		end
	end
	return wrapper
end

---创建task，tasklist中所有任务成功完成，task才完成
---@param taskList Task[]
---@return Task
function TaskHelper.createCombineTask(taskList)
	return TaskHelper.createSteps(function (task)
		for _, itemTask in ipairs(taskList) do
			itemTask:startAsChild(task)
			itemTask:appendCancelNotify(task)
		end
		return TaskAsyncOp.End
	end)
end

--- 合并出一个新的Task，每个task顺序都完成才完成
---@param taskList Task[]
---@return Task
function TaskHelper.createSequenceTask(taskList)
	return TaskHelper.createStepsEx(function (task, step)
		local itemTask = taskList[step]
		if itemTask then
			return task:completeSub(itemTask)
		else
			return TaskAsyncOp.End
		end
	end, function () return TaskAsyncOp.Stop end)
end

--- 任意一个完成就算完成，任意一个失败就算失败
---@param taskList Task[]
---@return Task
function TaskHelper.createAnyTask(taskList)
	return TaskHelper.createOneStepEx(function (_task)
		return function (task, resumeEntry)
			for _, itemTask in ipairs(taskList) do
				task:appendCancelNotify(itemTask)
				itemTask:continueWith(function(thisTask)
					if task:isActive() then
						task:finishTask(thisTask:getResult())
						for _, ta in ipairs(taskList) do
							if ta ~= thisTask then
								ta:cancel()
							end
						end

						resumeEntry()
					end
				end)
				:start()
			end
		end
	end, function () return TaskAsyncOp.Stop end)
end

---@param seconds number
---@return Task
function TaskHelper.waitForSeconds(seconds)
	assert(seconds >= 0, "Invalid seconds " .. tostring(seconds))
	return TaskHelper.createOneStepEx(function (_task)
		return function (task, resumeEntry)
			ScheduleService:DelayTime(task, function()
				if task:isActive() then
					resumeEntry()
				end
			end, seconds)
		end
	end, function () return TaskAsyncOp.Stop end)
end

---@param frames integer
---@return Task
function TaskHelper.waitForFrame(frames)
	assert(frames > 0, "Invalid frame number " .. tostring(frames))
	return TaskHelper.createOneStepEx(function (_task)
		return function (task, resumeEntry)
			ScheduleService:AddUpdater(task, function()
				if task:isActive() then
					resumeEntry()
				end
			end, true, frames)
		end
	end, function () return TaskAsyncOp.Stop end)
end

return TaskHelper