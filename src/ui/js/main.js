import { fetchAudioFiles, getDownloadUrl } from './api.js';

// 获取 HTML 里的元素
const tableBody = document.getElementById('tableBody');
const refreshBtn = document.getElementById('refreshBtn');

// 核心渲染函数
async function loadAndRenderData() {
    tableBody.innerHTML = '<tr><td colspan="5" class="loading">Pulling Data from Database...</td></tr>';

    // 调用 api.js 里的函数，真正发起网络请求
    const result = await fetchAudioFiles();

    if (!result) {
        tableBody.innerHTML = '<tr><td colspan="5" class="loading" style="color:#f48771;">Connection Failed!</td></tr>';
        return;
    }

    if (result.status === 'success') {
        renderTable(result.data);
    } else {
        tableBody.innerHTML = `<tr><td colspan="5" class="loading" style="color:#f48771;">Error: ${result.message}</td></tr>`;
    }
}

// 将 JSON 数组转化为 HTML 元素
function renderTable(dataArray) {
    tableBody.innerHTML = ''; // 清空加载提示

    if (dataArray.length === 0) {
        tableBody.innerHTML = '<tr><td colspan="5" class="loading">No Records in Database</td></tr>';
        return;
    }

    // 遍历后端的数组
    dataArray.forEach(item => {
        const tr = document.createElement('tr');

        // 动态拼装 <td>
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

    // 为刚刚生成的所有“下载”按钮绑定点击事件
    document.querySelectorAll('.download-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const fileId = e.target.getAttribute('data-id');
            // 打开新窗口，直接访问 C++ 的下载接口
            window.open(getDownloadUrl(fileId), '_blank');
        });
    });
}

// 绑定顶部的刷新按钮
refreshBtn.addEventListener('click', loadAndRenderData);

// 网页一打开，立刻执行一次请求
loadAndRenderData();