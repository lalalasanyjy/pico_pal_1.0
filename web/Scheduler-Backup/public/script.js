// 全局变量
let schedulesData = [];
let originalData = [];

// 主页面初始化
function initIndexPage() {
    loadSchedules();
    
    // 新增按钮事件
    document.getElementById('addSchedule').addEventListener('click', () => {
        window.location.href = 'edit.html';
    });
    
    // 保存按钮事件
    document.getElementById('saveBtn').addEventListener('click', saveChanges);
    
    // 恢复按钮事件
    document.getElementById('resetBtn').addEventListener('click', resetChanges);
}

// 编辑页面初始化
function initEditPage() {
    // 初始化日期选择器
    flatpickr('.datepicker', {
        locale: 'zh',
        dateFormat: 'Y-m-d',
        allowInput: true
    });
    
    // 是否重复切换事件
    document.getElementById('is_repeated').addEventListener('change', toggleRepeatFields);
    
    // 重复类型切换事件
    document.getElementById('repeat_type').addEventListener('change', updateWhichDayOptions);
    
    // 表单提交事件
    document.getElementById('scheduleForm').addEventListener('submit', handleFormSubmit);
    
    // 取消按钮事件
    document.getElementById('cancelBtn').addEventListener('click', () => {
        window.location.href = 'index.html';
    });
    
    // 加载编辑的日程数据
    const urlParams = new URLSearchParams(window.location.search);
    const id = urlParams.get('id');
    
    if (id) {
        document.getElementById('formTitle').textContent = '编辑日程';
        loadScheduleForEdit(id);
    } else {
        document.getElementById('formTitle').textContent = '新增日程';
        // 默认值
        document.getElementById('reminder_count').value = 1;
        document.getElementById('reminder_interval').value = 5;
    }
    
    // 初始隐藏/显示字段
    toggleRepeatFields();
}

// 加载日程列表
async function loadSchedules() {
    try {
        const response = await fetch('/api/schedules');
        const data = await response.json();
        schedulesData = data.schedules;
        originalData = JSON.parse(JSON.stringify(data.schedules)); // 深拷贝
        
        // 更新状态
        updateAllScheduleStatuses();
        
        displaySchedules();
    } catch (error) {
        console.error('加载日程失败:', error);
        alert('加载日程失败，请刷新重试');
    }
}

// 显示日程列表
function displaySchedules() {
    const scheduleList = document.getElementById('scheduleList');
    if (!scheduleList) return;
    
    scheduleList.innerHTML = '';
    
    if (schedulesData.length === 0) {
        scheduleList.innerHTML = '<div class="empty-message">暂无日程</div>';
        return;
    }
    
    schedulesData.forEach(schedule => {
        const scheduleItem = document.createElement('div');
        scheduleItem.className = 'schedule-item';
        
        scheduleItem.innerHTML = `
            <div class="schedule-info">
                <div class="schedule-name">
                    ${schedule.name}
                    <span class="schedule-status status-${getStatusClass(schedule.status)}">
                        ${schedule.status}
                    </span>
                </div>
                <div class="schedule-meta">
                    <span>${formatScheduleTime(schedule)}</span>
                    ${schedule.is_repeated ? `<span>重复: ${formatRepeatType(schedule.repeat_type)}</span>` : ''}
                </div>
            </div>
            <div class="schedule-actions">
                <button class="btn-secondary edit-btn" data-id="${schedule.id}">编辑</button>
                <button class="btn-danger delete-btn" data-id="${schedule.id}">删除</button>
            </div>
        `;
        
        scheduleList.appendChild(scheduleItem);
    });
    
    // 添加事件监听
    document.querySelectorAll('.edit-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const id = e.target.getAttribute('data-id');
            window.location.href = `edit.html?id=${id}`;
        });
    });
    
    document.querySelectorAll('.delete-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const id = parseInt(e.target.getAttribute('data-id'));
            deleteSchedule(id);
        });
    });
}

// 删除日程 (内存中)
function deleteSchedule(id) {
    schedulesData = schedulesData.filter(schedule => schedule.id !== id);
    displaySchedules();
}

// 保存更改到服务器
async function saveChanges() {
    try {
        const response = await fetch('/api/schedules', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                schedule_count: schedulesData.length,
                schedules: schedulesData
            })
        });
        
        const result = await response.json();
        
        if (result.success) {
            alert('保存成功');
            // 更新原始数据
            originalData = JSON.parse(JSON.stringify(schedulesData));
        } else {
            throw new Error('保存失败');
        }
    } catch (error) {
        console.error('保存失败:', error);
        alert('保存失败，请重试');
    }
}

// 恢复更改
function resetChanges() {
    schedulesData = JSON.parse(JSON.stringify(originalData));
    displaySchedules();
}

// 加载编辑的日程数据
async function loadScheduleForEdit(id) {
    try {
        const response = await fetch('/api/schedules');
        const data = await response.json();
        const schedule = data.schedules.find(s => s.id == id);
        
        if (schedule) {
            populateForm(schedule);
        }
    } catch (error) {
        console.error('加载日程失败:', error);
        alert('加载日程失败');
    }
}

// 填充表单数据
function populateForm(schedule) {
    document.getElementById('scheduleId').value = schedule.id;
    document.getElementById('name').value = schedule.name;
    document.getElementById('is_repeated').value = schedule.is_repeated;
    document.getElementById('repeat_type').value = schedule.repeat_type || '';
    document.getElementById('which_day').value = schedule.which_day || '';
    document.getElementById('start_date').value = schedule.start_date || '';
    document.getElementById('end_date').value = schedule.end_date || '';
    document.getElementById('exact_date').value = schedule.exact_date || '';
    document.getElementById('exact_time').value = schedule.exact_time;
    document.getElementById('reminder_count').value = schedule.reminder_count;
    document.getElementById('reminder_interval').value = schedule.reminder_interval;
    
    // 触发字段更新
    toggleRepeatFields();
    updateWhichDayOptions();
}

// 切换是否显示重复相关字段
function toggleRepeatFields() {
    const isRepeated = document.getElementById('is_repeated').value === 'true';
    const repeatFields = document.getElementById('repeatFields');
    const exactDateField = document.getElementById('exact_date').parentElement;
    
    if (isRepeated) {
        repeatFields.style.display = 'block';
        exactDateField.style.display = 'none';
        
        // 清除非重复任务的字段
        document.getElementById('exact_date').value = '';
    } else {
        repeatFields.style.display = 'none';
        exactDateField.style.display = 'block';
        
        // 清除重复任务的字段
        document.getElementById('repeat_type').value = '';
        document.getElementById('which_day').value = '';
        document.getElementById('start_date').value = '';
        document.getElementById('end_date').value = '';
    }
}

// 更新日期/星期选项
function updateWhichDayOptions() {
    const repeatType = document.getElementById('repeat_type').value;
    const whichDayGroup = document.getElementById('whichDayGroup');
    const whichDaySelect = document.getElementById('which_day');
    
    whichDaySelect.innerHTML = '';
    
    if (repeatType === 'week') {
        whichDayGroup.style.display = 'block';
        
        const days = [
            {value: 1, text: '星期一'},
            {value: 2, text: '星期二'},
            {value: 3, text: '星期三'},
            {value: 4, text: '星期四'},
            {value: 5, text: '星期五'},
            {value: 6, text: '星期六'},
            {value: 7, text: '星期日'}
        ];
        
        days.forEach(day => {
            const option = document.createElement('option');
            option.value = day.value;
            option.textContent = day.text;
            whichDaySelect.appendChild(option);
        });
    } else if (repeatType === 'month') {
        whichDayGroup.style.display = 'block';
        
        // 生成1-31日的选项
        for (let i = 1; i <= 31; i++) {
            const option = document.createElement('option');
            option.value = i;
            option.textContent = `${i}日`;
            whichDaySelect.appendChild(option);
        }
    } else {
        whichDayGroup.style.display = 'none';
    }
}

// 处理表单提交
async function handleFormSubmit(e) {
    e.preventDefault();
    
    const id = document.getElementById('scheduleId').value;
    const isNew = !id;
    
    const schedule = {
        id: isNew ? Date.now() : parseInt(id),
        name: document.getElementById('name').value,
        is_repeated: document.getElementById('is_repeated').value === 'true',
        repeat_type: document.getElementById('is_repeated').value === 'true' ? 
                     document.getElementById('repeat_type').value : null,
        which_day: document.getElementById('is_repeated').value === 'true' && 
                  ['week', 'month'].includes(document.getElementById('repeat_type').value) ?
                  parseInt(document.getElementById('which_day').value) : null,
        start_date: document.getElementById('is_repeated').value === 'true' ? 
                    document.getElementById('start_date').value : null,
        end_date: document.getElementById('is_repeated').value === 'true' ? 
                  document.getElementById('end_date').value || null : null,
        exact_date: document.getElementById('is_repeated').value === 'false' ? 
                    document.getElementById('exact_date').value : null,
        exact_time: document.getElementById('exact_time').value,
        reminder_count: parseInt(document.getElementById('reminder_count').value),
        reminder_interval: parseInt(document.getElementById('reminder_interval').value),
        status: "未开始"
    };
    
    // 验证数据
    if (!validateSchedule(schedule)) {
        return;
    }
    
    try {
        // 获取当前数据
        const response = await fetch('/api/schedules');
        const data = await response.json();
        let schedules = data.schedules;
        
        if (isNew) {
            schedules.push(schedule);
        } else {
            schedules = schedules.map(s => s.id == schedule.id ? schedule : s);
        }
        
        // 保存到服务器
        const saveResponse = await fetch('/api/schedules', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                schedule_count: schedules.length,
                schedules: schedules
            })
        });
        
        const result = await saveResponse.json();
        
        if (result.success) {
            alert('保存成功');
            window.location.href = 'index.html';
        } else {
            throw new Error('保存失败');
        }
    } catch (error) {
        console.error('保存失败:', error);
        alert('保存失败，请重试');
    }
}

// 验证日程数据
function validateSchedule(schedule) {
    if (!schedule.name) {
        alert('请输入日程名称');
        return false;
    }
    
    if (!schedule.exact_time) {
        alert('请选择时间');
        return false;
    }
    
    if (schedule.is_repeated) {
        if (!schedule.repeat_type) {
            alert('请选择重复类型');
            return false;
        }
        
        if (!schedule.start_date) {
            alert('请选择开始日期');
            return false;
        }
        
        // 验证每月重复的日期是否有效
        if (schedule.repeat_type === 'month' && schedule.which_day) {
            const startDate = new Date(schedule.start_date);
            const maxDay = new Date(
                startDate.getFullYear(), 
                startDate.getMonth() + 1, 
                0
            ).getDate();
            
            if (schedule.which_day > maxDay) {
                alert(`所选月份没有 ${schedule.which_day} 日`);
                return false;
            }
        }
    } else {
        if (!schedule.exact_date) {
            alert('请选择具体日期');
            return false;
        }
    }
    
    return true;
}

// 更新所有日程状态
function updateAllScheduleStatuses() {
    const now = new Date();
    
    schedulesData.forEach(schedule => {
        if (schedule.status === "已结束") return;
        
        let scheduleDate;
        
        if (schedule.is_repeated) {
            // 对于重复任务，检查下一次发生的时间
            if (!schedule.start_date) return;
            
            const startDate = new Date(schedule.start_date);
            const timeParts = schedule.exact_time.split(':');
            const hours = parseInt(timeParts[0]);
            const minutes = parseInt(timeParts[1]);
            
            // 设置基准时间为今天的该时间
            scheduleDate = new Date(
                now.getFullYear(),
                now.getMonth(),
                now.getDate(),
                hours,
                minutes
            );
            
            // 调整日期以匹配重复规则
            if (schedule.repeat_type === 'week' && schedule.which_day) {
                const targetDay = schedule.which_day % 7; // 确保在0-6范围内
                const currentDay = scheduleDate.getDay() || 7; // 0是周日，我们改为7
                
                let daysToAdd = targetDay - currentDay;
                if (daysToAdd < 0) daysToAdd += 7;
                
                scheduleDate.setDate(scheduleDate.getDate() + daysToAdd);
            } else if (schedule.repeat_type === 'month' && schedule.which_day) {
                scheduleDate.setDate(schedule.which_day);
                
                // 如果已经过了这个月的这一天，跳到下个月
                if (scheduleDate < now) {
                    scheduleDate.setMonth(scheduleDate.getMonth() + 1);
                }
            }
            
            // 检查是否在结束日期之前
            if (schedule.end_date) {
                const endDate = new Date(schedule.end_date);
                endDate.setHours(23, 59, 59, 999);
                
                if (scheduleDate > endDate) {
                    schedule.status = "已结束";
                    return;
                }
            }
            
            // 检查是否在开始日期之后
            if (scheduleDate < startDate) {
                schedule.status = "未开始";
                return;
            }
        } else {
            // 非重复任务
            if (!schedule.exact_date) return;
            
            scheduleDate = new Date(`${schedule.exact_date}T${schedule.exact_time}`);
        }
        
        // 设置状态
        if (now > scheduleDate) {
            schedule.status = "已结束";
        } else if (now.getTime() > scheduleDate.getTime() - 3600000) { // 1小时前
            schedule.status = "进程中";
        } else {
            schedule.status = "未开始";
        }
    });
}

// 获取状态类名
function getStatusClass(status) {
    return status === "未开始" ? "not-started" : 
           status === "进程中" ? "in-progress" : "completed";
}

// 格式化日程时间显示
function formatScheduleTime(schedule) {
    if (schedule.is_repeated) {
        if (schedule.repeat_type === 'day') {
            return `每天 ${schedule.exact_time}`;
        } else if (schedule.repeat_type === 'week') {
            const days = ['日', '一', '二', '三', '四', '五', '六'];
            return `每周${days[(schedule.which_day - 1) % 7]} ${schedule.exact_time}`;
        } else if (schedule.repeat_type === 'month') {
            return `每月${schedule.which_day}日 ${schedule.exact_time}`;
        }
    } else {
        return `${schedule.exact_date} ${schedule.exact_time}`;
    }
    return schedule.exact_time;
}

// 格式化重复类型显示
function formatRepeatType(type) {
    const types = {
        'day': '每日',
        'week': '每周',
        'month': '每月'
    };
    return types[type] || type;
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function() {
    if (document.getElementById('scheduleList')) {
        initIndexPage();
    } else if (document.getElementById('scheduleForm')) {
        initEditPage();
    }
});