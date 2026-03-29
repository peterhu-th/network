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
        const id = item.id || 'N/A';
        const filePath = item.filePath || item.path || 'Unknown';
        const extMatch = filePath.match(/\.[0-9a-z]+$/i);
        const ext = extMatch ? extMatch[0] : '.wav';
        const rawSize = parseInt(item.fileSize || item.size || 0, 10);
        const sizeStr = !isNaN(rawSize) ? (rawSize / 1024).toFixed(2) + ' KB' : '0 KB';
        const rawTime = item.generationTime || item.created_at || '';
        const timeStr = rawTime ? new Date(rawTime).toLocaleString() : 'Unknown';

        tr.innerHTML = `
            <td style="font-family: monospace; color: #ce9178;">${id}</td>
            <td title="${filePath}">${filePath}</td>
            <td>${sizeStr}</td>
            <td>${timeStr}</td>
            <td>
                <button class="download-wav-btn" data-id="${id}" data-time="${rawTime}" style="margin-right: 5px;">WAV</button>
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

function formatFileNameTime(rawTime) {
    if (!rawTime) return `record_${Date.now()}`;

    const date = new Date(rawTime);
    if (isNaN(date.getTime())) return `record_${Date.now()}`;

    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    const seconds = String(date.getSeconds()).padStart(2, '0');

    return `${year}年${month}月${day}日${hours}时${minutes}分${seconds}秒`;
}

tableBody.addEventListener('click', async (e) => {
    if (e.target.classList.contains('download-wav-btn')) {
        const btn = e.target;
        btn.disabled = true;
        btn.innerText = "Downloading...";

        try {
            const fileId = btn.getAttribute('data-id');
            const rawTime = btn.getAttribute('data-time');
            const ext = btn.getAttribute('data-ext') || '.wav';
            const fileName = formatFileNameTime(rawTime);
            const input = document.getElementById('speedLimitInput');
            const speed = parseInt(input?.value) || 0;

            const blob = await downloadAudioFile(fileId, speed);

            const blobUrl = window.URL.createObjectURL(blob);
            triggerDownload(blobUrl, `${fileName}${ext}`);
            window.URL.revokeObjectURL(blobUrl);

        } catch (error) {
            alert("Failed to download file: " + error.message);
        } finally {
            btn.disabled = false;
            btn.innerText = "WAV";
        }
    }
});

function renderPagination() {
    const totalPages = Math.ceil(totalRecords / pageSize) || 1;
    pageInfo.innerText = `Page ${currentPage} / ${totalPages}`;
    prevPageBtn.disabled = currentPage <= 1;
    nextPageBtn.disabled = currentPage >= totalPages;
}

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

loadAndRenderData().catch(console.error);