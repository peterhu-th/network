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
                <button class="download-btn" data-id="${item.id}">Download File</button>
            </td>
        `;
        tableBody.appendChild(tr);
    });
}

// 采用事件委托 (Event Delegation) 统一管理点击事件，告别此前给每个 .download-btn 分配单独事件的做法以防内存泄漏
tableBody.addEventListener('click', (e) => {
    if (e.target.classList.contains('download-btn')) {
        const fileId = e.target.getAttribute('data-id');
        window.open(getDownloadUrl(fileId), '_blank');
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