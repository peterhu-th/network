<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import axios from 'axios'
import { ElMessage } from 'element-plus'

const router = useRouter()

const SERVER_URL = `http://${window.location.hostname}:8080`

// 响应式数据：绑定到表格上
const tableData = ref([])
const loading = ref(false)

// 过滤条件模型
const filterForm = reactive({
  format: '',
  dateRange: null as [Date, Date] | null
})

// 获取前端本地保存的 Token（实际应用中应由登录接口返回后保存至 localStorage）
const getToken = () => {
  return localStorage.getItem('token') || ''
}

// 获取文件列表
const fetchFiles = async (isRefresh = false) => {
  loading.value = true
  try {
    const token = getToken()
    let requestParams: any = { limit: 50, offset: 0 }
    
    // 强制扫描参数
    if (isRefresh === true) {
      requestParams.forceScan = 'true'
    }
    
    // 注入格式与时间过滤
    if (filterForm.format) {
      requestParams.format = filterForm.format
    }
    if (filterForm.dateRange && filterForm.dateRange.length === 2 && filterForm.dateRange[0] && filterForm.dateRange[1]) {
      const start = new Date(filterForm.dateRange[0])
      const end = new Date(filterForm.dateRange[1])
      end.setHours(23, 59, 59, 999)
      requestParams.startTime = start.toISOString()
      requestParams.endTime = end.toISOString()
    }

    const response = await axios.get(`${SERVER_URL}/files`, {
      headers: { 'Authorization': `Bearer ${token}` },
      params: requestParams
    })

    if (response.data.code === 20000) {
      tableData.value = response.data.data.list
    } else {
      ElMessage.error(response.data.message || '获取列表失败')
    }
  } catch (error: any) {
    console.error(error)
    if (error.response && error.response.status === 401) {
      ElMessage.error('登录失效（Token 验证失败），请重新登录')
      localStorage.removeItem('token')
      router.push('/login')
    } else {
      ElMessage.error('网络请求失败，请检查 C++ 后端是否启动')
    }
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

// 退出登录逻辑
const handleLogout = () => {
  localStorage.removeItem('token')
  localStorage.removeItem('username')
  router.push('/login')
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
          <div style="display: flex; gap: 10px; align-items: center; flex-wrap: wrap;">
            <!-- 筛选器 -->
            <el-select v-model="filterForm.format" placeholder="文件类型" clearable style="width: 120px" @change="() => fetchFiles(false)">
              <el-option label="所有" value="" />
              <el-option label="MP3" value="mp3" />
              <el-option label="WAV" value="wav" />
              <el-option label="M4A" value="m4a" />
            </el-select>

            <el-date-picker
              v-model="filterForm.dateRange"
              type="daterange"
              unlink-panels
              range-separator="至"
              start-placeholder="开始日期"
              end-placeholder="结束日期"
              style="width: 250px"
              @change="() => fetchFiles(false)"
            />

            <!-- 按钮 -->
            <el-button type="primary" :icon="'Refresh'" @click="() => fetchFiles(true)">搜索 / 刷新</el-button>
            <el-button type="danger" @click="handleLogout">退出登录</el-button>
          </div>
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