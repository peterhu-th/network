import { fetchAudioFiles, getDownloadUrl } from './api.js';

// 获取 HTML 里的元素
const tableBody = document.getElementById('tableBody');
const refreshBtn = document.getElementById('refreshBtn');

// === UI 重构说明 === 
// 用于管理分页的 DOM 节点
const prevPageBtn = document.getElementById('prevPageBtn');
const nextPageBtn = document.getElementById('nextPageBtn');
const pageInfo = document.getElementById('pageInfo');

// --- Model 层 ---
// 全局状态管理
let currentPage = 1;
const pageSize = 20; // 设定每页条目大小为20
let totalRecords = 0;

// --- Controller 层 ---
// 核心数据抓取函数
async function loadAndRenderData() {
    tableBody.innerHTML = '<tr><td colspan="5" class="loading">Pulling Data from Database...</td></tr>';

    // 换算当前页码与后端所需的 offset
    const offset = (currentPage - 1) * pageSize;

    // 采用携带页面限制的拉取策略
    const result = await fetchAudioFiles(pageSize, offset);

    if (!result) {
        tableBody.innerHTML = '<tr><td colspan="5" class="loading" style="color:#f48771;">Connection Failed!</td></tr>';
        return;
    }

    if (result.status === 'success') {
        // 同步总记录数，计算总页数并渲染
        totalRecords = result.total || 0;
        renderTable(result.data);
        renderPagination();
    } else {
        tableBody.innerHTML = `<tr><td colspan="5" class="loading" style="color:#f48771;">Error: ${result.message}</td></tr>`;
    }
}

// --- View 层 ---
// 渲染表格 (DOM 构建)
function renderTable(dataArray) {
    tableBody.innerHTML = ''; // 清空加载提示

    if (!dataArray || dataArray.length === 0) {
        tableBody.innerHTML = '<tr><td colspan="5" class="loading">No Records in Database</td></tr>';
        return;
    }

    // 遍历当前页的数据组装
    dataArray.forEach(item => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td style="font-family: monospace; color: #ce9178;">${item.id}</td>
            <td>${item.filePath}</td>
            <td>${item.fileSize}</td>
            <td>${item.generationTime}</td>
            <td>
                <button class="download-wav-btn" data-id="${item.id}" style="margin-right: 5px;">WAV</button>
                <button class="download-both-btn" data-id="${item.id}">WAV + JSON</button>
            </td>
        `;
        tableBody.appendChild(tr);
    });
}

// 辅助函数：通过隐式 a 标签，并注入 HTML5 download 属性来触发静默下载，不弹新窗口
function triggerDownload(url, forcedFilename) {
    const a = document.createElement('a');
    a.style.display = 'none';
    a.href = url;
    // 不再使用 target="_blank"，因为连续弹出多窗口必被浏览器拦截
    // 添加 download 属性强制声明这是一个下载动作
    a.setAttribute('download', forcedFilename || 'download');

    document.body.appendChild(a);
    a.click();

    // 延迟少许移除以确保浏览器成功捕获事件
    setTimeout(() => {
        document.body.removeChild(a);
    }, 100);
}

function getSpeedParam() {
    const input = document.getElementById('speedLimitInput');
    const val = parseInt(input?.value) || 0;
    return val > 0 ? `&speed=${val}` : '';
}

tableBody.addEventListener('click', (e) => {
    if (e.target.classList.contains('download-wav-btn')) {
        const fileId = e.target.getAttribute('data-id');
        triggerDownload(getDownloadUrl(fileId) + getSpeedParam(), `record_${fileId}.wav`);
    } else if (e.target.classList.contains('download-both-btn')) {
        const fileId = e.target.getAttribute('data-id');
        const speed = getSpeedParam();
        const wavUrl = getDownloadUrl(fileId) + speed;
        const jsonUrl = getDownloadUrl(fileId) + "&type=json" + speed;

        // 通过间隔同步调用，并在内部加上 download 属性。
        // 不加 target="_blank"，避免进入浏览器的恶意弹窗防御域 (Popup Blocker)。
        // 加入 800ms 延时，确保第一个 WAV 请求的响应头 (Content-Disposition: attachment) 
        // 已经返回并接管了当前页面的下载流控制器，此时再发第二个请求 JSON，
        // 浏览器就不会因为“连续两次试图导航当前页面”而默认中断(Abort)掉第一次的 WAV 请求了。

        triggerDownload(wavUrl, `record_${fileId}.wav`);
        setTimeout(() => {
            triggerDownload(jsonUrl, `record_${fileId}.json`);
        }, 50);
    }
});

// 渲染底部分页指示器与按钮状态
function renderPagination() {
    const totalPages = Math.ceil(totalRecords / pageSize) || 1;
    pageInfo.innerText = `Page ${currentPage} / ${totalPages}`;

    // 如果无法前进后退，将按钮灰显禁用
    prevPageBtn.disabled = currentPage <= 1;
    nextPageBtn.disabled = currentPage >= totalPages;
}

// 绑定底部分页的上下页控制事件
prevPageBtn.addEventListener('click', () => {
    if (currentPage > 1) {
        currentPage--;
        loadAndRenderData();
    }
});

nextPageBtn.addEventListener('click', () => {
    const totalPages = Math.ceil(totalRecords / pageSize) || 1;
    if (currentPage < totalPages) {
        currentPage++;
        loadAndRenderData();
    }
});

// 绑定顶部的刷新按钮（回到第一页重新拉取）
refreshBtn.addEventListener('click', () => {
    currentPage = 1;
    loadAndRenderData();
});

// 网页一打开，立刻请求第一页
loadAndRenderData();