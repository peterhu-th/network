<script setup lang="ts">
import { ref, onMounted } from 'vue'
import axios from 'axios'
import { ElMessage } from 'element-plus'

const SERVER_URL = `http://${window.location.hostname}:8080/api`

// 响应式数据：绑定到表格上
const tableData = ref([])
const loading = ref(false)

// 获取前端本地保存的 Token（实际应用中应由登录接口返回后保存至 localStorage）
const getToken = () => {
  return localStorage.getItem('token') || ''
}

// 获取文件列表
const fetchFiles = async () => {
  loading.value = true
  try {
    const token = getToken()
    const response = await axios.get(`${SERVER_URL}/files`, {
      headers: { 'Authorization': `Bearer ${token}` },
      params: { limit: 50, offset: 0 }
    })

    if (response.data.code === 20000) {
      tableData.value = response.data.data.list
    } else {
      ElMessage.error(response.data.message || '获取列表失败')
    }
  } catch (error) {
    console.error(error)
    ElMessage.error('网络请求失败，请检查 C++ 后端是否启动')
  } finally {
    loading.value = false
  }
}

// 生成可读的文件名
const generateTimeFileName = (extension = 'mp3') => {
  const now = new Date()
  const year = now.getFullYear()
  const month = String(now.getMonth() + 1).padStart(2, '0')
  const day = String(now.getDate()).padStart(2, '0')
  const hours = String(now.getHours()).padStart(2, '0')
  const minutes = String(now.getMinutes()).padStart(2, '0')
  const seconds = String(now.getSeconds()).padStart(2, '0')
  return `录音_${year}${month}${day}_${hours}${minutes}${seconds}.${extension}`
}

// Blob 异步流式下载
const handleDownload = async (row: any) => {
  try {
    ElMessage.info(`正在下载文件 ID: ${row.id}...`)
    const token = getToken()

    const response = await axios({
      url: `${SERVER_URL}/download`,
      method: 'GET',
      params: { id: row.id },
      responseType: 'blob',
      headers: { 'Authorization': `Bearer ${token}` }
    })

    // 将二进制流塞入内存对象，并创建虚拟链接下载
    const blob = new Blob([response.data], { type: response.headers['content-type'] || 'audio/mpeg' })
    const downloadUrl = window.URL.createObjectURL(blob)
    const link = document.createElement('a')

    link.href = downloadUrl
    link.download = generateTimeFileName('mp3') // 强制重命名
    document.body.appendChild(link)
    link.click() // 模拟点击
    document.body.removeChild(link)
    window.URL.revokeObjectURL(downloadUrl)
    ElMessage.success('下载成功！')

  } catch (error) {
    console.error(error)
    ElMessage.error('下载失败，文件可能不存在或无权限')
  }
}

// 数据格式化函数
const formatStartTime = (row: any, column: any, cellValue: string) => {
  if (!cellValue) return '-'
  const endDate = new Date(cellValue)
  const durationSeconds = row.duration || 0
  const startDate = new Date(endDate.getTime() - durationSeconds * 1000)
  return startDate.toLocaleString('zh-CN', { hour12: false }).replace(/\//g, '-')
}

const formatFileSize = (row: any, column: any, cellValue: number) => {
  if (cellValue === null || cellValue === undefined) return '0.00 MB'
  if (cellValue === 0) return '0.00 MB'
  return (cellValue / (1024 * 1024)).toFixed(2) + ' MB'
}

// 页面加载时自动请求数据
onMounted(() => {
  fetchFiles()
})
</script>

<template>
  <div class="home-container">
    <el-card class="box-card">
      <template #header>
        <div class="card-header">
          <h2>雷达音频管理控制台</h2>
          <el-button type="primary" :icon="'Refresh'" @click="fetchFiles">刷新列表</el-button>
        </div>
      </template>

      <!-- 数据表格 -->
      <!-- 数据表格 -->
      <el-table :data="tableData" v-loading="loading" stripe border style="width: 100%">
        <el-table-column prop="generationTime" label="起始时间" :formatter="formatStartTime" min-width="180" align="center" />

        <el-table-column prop="duration" label="时长 (秒)" width="120" align="center" />

        <el-table-column prop="fileSize" label="文件大小" :formatter="formatFileSize" width="150" align="center" />

        <!-- 操作列 -->
        <el-table-column label="操作" width="150" align="center">
          <template #default="scope">
            <el-button type="success" :icon="'Download'" size="small" @click="handleDownload(scope.row)">
              下载
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-card>
  </div>
</template>

<style scoped>
.home-container {
  padding: 20px;
  max-width: 1200px;
  margin: 0 auto;
}
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}
h2 {
  margin: 0;
  color: #303133;
}
</style>