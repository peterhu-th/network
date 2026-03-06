import { fetchAudioFiles, getDownloadUrl } from './api.js';

// 获取 HTML 里的元素
const tableBody = document.getElementById('tableBody');
const refreshBtn = document.getElementById('refreshBtn');

// 用于管理分页的 DOM 节点
const prevPageBtn = document.getElementById('prevPageBtn');
const nextPageBtn = document.getElementById('nextPageBtn');
const pageInfo = document.getElementById('pageInfo');

// --- Model 层 ---
// 全局状态管理
let currentPage = 1;
const pageSize = 20;
let totalRecords = 0;

// --- Controller 层 ---
// 核心数据抓取函数
async function loadAndRenderData() {
    tableBody.innerHTML = '<tr><td colspan="5" class="loading">Pulling Data from Database...</td></tr>';

    try {
        const offset = (currentPage - 1) * pageSize;
        const result = await fetchAudioFiles(pageSize, offset);

        if (!result) {
            tableBody.innerHTML = '<tr><td colspan="5" class="loading" style="color:#f48771;">Connection Failed!</td></tr>';
            return;
        }

        // 兼容不同的后端状态码习惯
        if (result.status === 'success' || result.code === 200) {
            totalRecords = result.total || 0;
            renderTable(result.data);
            renderPagination();
        } else {
            tableBody.innerHTML = `<tr><td colspan="5" class="loading" style="color:#f48771;">Error: ${result.message || 'Unknown Error'}</td></tr>`;
        }
    } catch (error) {
        // 增加兜底异常捕获
        console.error("Data Fetch Exception:", error);
        tableBody.innerHTML = `<tr><td colspan="5" class="loading" style="color:#f48771;">Exception: ${error.message}</td></tr>`;
    }
}

// --- View 层 ---
// 渲染表格 (DOM 构建)
function renderTable(dataArray) {
    tableBody.innerHTML = '';

    if (!dataArray || dataArray.length === 0) {
        tableBody.innerHTML = '<tr><td colspan="5" class="loading">No Records in Database</td></tr>';
        return;
    }

    dataArray.forEach(item => {
        const tr = document.createElement('tr');

        // 【修复 1：全面解析并格式化后端字段，同时兼容不同的后端键名】
        const id = item.id || 'N/A';
        const filePath = item.filePath || item.path || 'Unknown';

        // 解析容量 (Byte -> KB)
        const rawSize = parseInt(item.fileSize || item.size || 0, 10);
        const sizeStr = !isNaN(rawSize) ? (rawSize / 1024).toFixed(2) + ' KB' : '0 KB';

        // 解析时间 (ISO String/Timestamp -> Local Datetime)
        const rawTime = item.generationTime || item.created_at || '';
        const timeStr = rawTime ? new Date(rawTime).toLocaleString() : 'Unknown';

        tr.innerHTML = `
            <td style="font-family: monospace; color: #ce9178;">${id}</td>
            <td title="${filePath}">${filePath}</td>
            <td>${sizeStr}</td>
            <td>${timeStr}</td>
            <td>
                <button class="download-wav-btn" data-id="${id}" style="margin-right: 5px;">WAV</button>
                <button class="download-both-btn" data-id="${id}">WAV + JSON</button>
            </td>
        `;
        tableBody.appendChild(tr);
    });
}

// 辅助函数：触发静默下载
function triggerDownload(url, forcedFilename) {
    const a = document.createElement('a');
    a.style.display = 'none';
    a.href = url;
    a.setAttribute('download', forcedFilename || 'download');

    document.body.appendChild(a);
    a.click();

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

        triggerDownload(wavUrl, `record_${fileId}.wav`);
        setTimeout(() => {
            triggerDownload(jsonUrl, `record_${fileId}.json`);
        }, 50);
    }
});

function renderPagination() {
    const totalPages = Math.ceil(totalRecords / pageSize) || 1;
    pageInfo.innerText = `Page ${currentPage} / ${totalPages}`;
    prevPageBtn.disabled = currentPage <= 1;
    nextPageBtn.disabled = currentPage >= totalPages;
}

// 【修复 2：所有调用 async 函数的地方加入 await 和 catch，防止 Promise 被忽略抛出异常】
prevPageBtn.addEventListener('click', async () => {
    if (currentPage > 1) {
        currentPage--;
        await loadAndRenderData().catch(console.error);
    }
});

nextPageBtn.addEventListener('click', async () => {
    const totalPages = Math.ceil(totalRecords / pageSize) || 1;
    if (currentPage < totalPages) {
        currentPage++;
        await loadAndRenderData().catch(console.error);
    }
});

refreshBtn.addEventListener('click', async () => {
    currentPage = 1;
    await loadAndRenderData().catch(console.error);
});

// 网页一打开，立刻请求第一页，并挂载全局异常捕获
loadAndRenderData().catch(console.error);