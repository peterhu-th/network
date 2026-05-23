<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import axios from 'axios'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()

const SERVER_URL = '/api'

// 响应式数据：绑定到表格上
const tableData = ref([])
const loading = ref(false)

// 分页与选择数据
const currentPage = ref(1)
const pageSize = ref(24)
const totalCount = ref(0)
const selectedIds = ref<number[]>([])

// 批量下载整页的转圈模式
const downloadingMode = ref(false)

// 过滤条件模型
const filterForm = reactive({
  format: '',
  dateRange: null as [Date, Date] | null
})

// 获取前端本地保存的 Token（实际应用中应由登录接口返回后保存至 localStorage）
const getToken = () => {
  return localStorage.getItem('token') || ''
}

// 角色判断
const isAdmin = ref(localStorage.getItem('role') === '1')

// 管理员面板相关状态
const adminDialogVisible = ref(false)
const guestsData = ref([])
const guestsLoading = ref(false)

// 获取文件列表
const fetchFiles = async (isRefresh = false) => {
  loading.value = true
  try {
    const token = getToken()
    let requestParams: any = {
      limit: pageSize.value,
      offset: (currentPage.value - 1) * pageSize.value
    }

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
      start.setHours(0, 0, 0, 0)
      const end = new Date(filterForm.dateRange[1])
      end.setHours(23, 59, 59, 999)
      const formatLocalISO = (date: Date) => {
        const pad = (n: number) => String(n).padStart(2, '0')
        return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}T${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`
      }
      requestParams.startTime = formatLocalISO(start)
      requestParams.endTime = formatLocalISO(end)
    }

    const response = await axios.get(`${SERVER_URL}/files`, {
      headers: { 'Authorization': `Bearer ${token}` },
      params: requestParams
    })

    if (response.data.code === 20000) {
      tableData.value = response.data.data.list
      totalCount.value = response.data.data.total
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
    ElMessage.error('下载失败，文件不存在或无权限')
  }
}

// 勾选状态监听
const handleSelectionChange = (selection: any[]) => {
  selectedIds.value = selection.map(row => row.id)
}

// 分页变化监听
const handlePageChange = (val: number) => {
  currentPage.value = val
  fetchFiles(false)
}

// 提取公共的异步轮询下载方法
const performBatchDownload = async (ids: number[]) => {
  if (ids.length === 0) {
    ElMessage.warning('当前列表中没有可供下载的文件')
    return
  }

  const token = getToken()
  downloadingMode.value = true

  const pollStatus = async () => {
    try {
      const response = await axios.post(`${SERVER_URL}/download/batch`, { ids }, {
        headers: { 'Authorization': `Bearer ${token}` }
      })

      if (response.data.code !== 20000) {
        throw new Error(response.data.message)
      }

      const { status, url } = response.data.data

      if (status === 'Processing') {
        // 每 1.5 秒轮询一次
        setTimeout(pollStatus, 3000)
      } else if (status === 'Completed' && url) {
        const fileResponse = await axios.get(`${SERVER_URL}${url}`, {
          responseType: 'blob',
          headers: { 'Authorization': `Bearer ${token}` }
        })

        const blob = new Blob([fileResponse.data], { type: fileResponse.headers['content-type'] || 'application/zip' })
        const downloadUrl = window.URL.createObjectURL(blob)
        const link = document.createElement('a')

        link.href = downloadUrl
        link.download = generateTimeFileName('zip') // 下载成 zip 压缩包
        document.body.appendChild(link)
        link.click()
        document.body.removeChild(link)
        window.URL.revokeObjectURL(downloadUrl)

        ElMessage.success('批量下载成功！')
        downloadingMode.value = false
      } else {
        throw new Error('批量打包意外中断或失败')
      }
    } catch (error: any) {
      console.error(error)
      ElMessage.error(error.message || '批量下载失败，可能网络中断或服务器错误')
      downloadingMode.value = false
    }
  }

  // 第一次激发出错尝试与注册轮询机制
  pollStatus()
}

// 批量下载当前页动作
const handleBatchDownloadPage = () => {
  const ids = tableData.value.map((row: any) => row.id)
  performBatchDownload(ids)
}

// 批量下载选中项动作
const handleBatchDownloadSelected = () => {
  performBatchDownload(selectedIds.value)
}

// 退出登录逻辑
const handleLogout = () => {
  localStorage.removeItem('token')
  localStorage.removeItem('email')
  localStorage.removeItem('role')
  localStorage.removeItem('id')
  router.push('/login')
}

// === 管理员功能 ===

// 删除文件
const handleDeleteFile = async (row: any) => {
  try {
    await ElMessageBox.confirm(`确定要永久删除文件 ID: ${row.id} 吗？此操作不可逆！`, '警告', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    })

    const response = await axios.delete(`${SERVER_URL}/files/${row.id}`, {
      headers: { 'Authorization': `Bearer ${getToken()}` }
    })

    if (response.data.code === 20000) {
      ElMessage.success('文件已删除')
      fetchFiles(false)
    } else {
      ElMessage.error(response.data.message || '删除失败')
    }
  } catch (error: any) {
    if (error !== 'cancel') {
      console.error(error)
      ElMessage.error(error.response?.data?.message || '删除失败')
    }
  }
}

// 获取访客列表
const fetchGuests = async () => {
  guestsLoading.value = true
  try {
    const response = await axios.get(`${SERVER_URL}/admin/guests`, {
      headers: { 'Authorization': `Bearer ${getToken()}` }
    })
    if (response.data.code === 20000) {
      guestsData.value = response.data.data
    } else {
      ElMessage.error(response.data.message || '获取访客列表失败')
    }
  } catch (error: any) {
    console.error(error)
    ElMessage.error(error.response?.data?.message || '网络请求失败')
  } finally {
    guestsLoading.value = false
  }
}

// 打开管理员面板
const openAdminPanel = () => {
  adminDialogVisible.value = true
  fetchGuests()
}

// 封停访客
const handleRevokeGuest = async (row: any) => {
  try {
    await ElMessageBox.confirm(`确定要封停访客 ${row.email} 吗？此操作将导致其无法登录。`, '警告', {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    })

    const response = await axios.put(`${SERVER_URL}/admin/guests/${row.id}/revoke`, {}, {
      headers: { 'Authorization': `Bearer ${getToken()}` }
    })

    if (response.data.code === 20000) {
      ElMessage.success(`访客 ${row.email} 已被封停`)
      fetchGuests() // 刷新列表
    } else {
      ElMessage.error(response.data.message || '操作失败')
    }
  } catch (error: any) {
    if (error !== 'cancel') {
      console.error(error)
      ElMessage.error(error.response?.data?.message || '操作失败')
    }
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
          <h2>雷达音频下载</h2>
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
            <el-button type="warning" :icon="'Download'" @click="handleBatchDownloadPage" element-loading-text="📦 批量打包中，请耐心等待..." v-loading.fullscreen.lock="downloadingMode">下载本页</el-button>
            <el-button
                :type="selectedIds.length === 0 ? 'info' : 'primary'"
                :disabled="selectedIds.length === 0"
                :icon="'Download'"
                @click="handleBatchDownloadSelected"
                element-loading-text="📦 正在打包选中文件..."
                v-loading.fullscreen.lock="downloadingMode">
              下载选中项 {{ selectedIds.length > 0 ? `(${selectedIds.length})` : '' }}
            </el-button>
            <el-button v-if="isAdmin" type="danger" :icon="'Setting'" @click="openAdminPanel">管理面板</el-button>
            <el-button type="info" @click="handleLogout">退出登录</el-button>
          </div>
        </div>
      </template>

      <!-- 数据表格 -->
      <!-- 数据表格 -->
      <el-table :data="tableData" v-loading="loading" stripe border style="width: 100%" @selection-change="handleSelectionChange">
        <el-table-column type="selection" width="55" align="center" />

        <el-table-column prop="generationTime" label="起始时间" :formatter="formatStartTime" min-width="180" align="center" />

        <el-table-column prop="duration" label="时长 (秒)" width="120" align="center" />

        <el-table-column prop="fileSize" label="文件大小" :formatter="formatFileSize" width="150" align="center" />

        <!-- 操作列 -->
        <el-table-column label="操作" width="200" align="center">
          <template #default="scope">
            <el-button type="success" :icon="'Download'" size="small" @click="handleDownload(scope.row)">
              下载
            </el-button>
            <el-button v-if="isAdmin" type="danger" :icon="'Delete'" size="small" @click="handleDeleteFile(scope.row)">
              删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>

      <!-- 分页区域 -->
      <div style="display: flex; justify-content: flex-end; margin-top: 20px;">
        <el-pagination
          background
          layout="total, prev, pager, next"
          :total="totalCount"
          :page-size="pageSize"
          v-model:current-page="currentPage"
          @current-change="handlePageChange"
        />
      </div>
    </el-card>

    <!-- 管理员面板弹窗 -->
    <el-dialog v-model="adminDialogVisible" title="访客管理" width="800px">
      <el-table :data="guestsData" v-loading="guestsLoading" stripe border style="width: 100%">
        <el-table-column prop="id" label="ID" width="180" align="center" />
        <el-table-column prop="email" label="邮箱" min-width="200" align="center" />
        <el-table-column prop="createdAt" label="注册时间" width="180" align="center">
          <template #default="scope">
            {{ new Date(scope.row.createdAt).toLocaleString('zh-CN', { hour12: false }).replace(/\//g, '-') }}
          </template>
        </el-table-column>
        <el-table-column prop="status" label="状态" width="100" align="center">
          <template #default="scope">
            <el-tag :type="scope.row.status === 1 ? 'success' : 'danger'">
              {{ scope.row.status === 1 ? '正常' : '已封停' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="120" align="center">
          <template #default="scope">
            <el-button
              type="danger"
              size="small"
              :disabled="scope.row.status !== 1"
              @click="handleRevokeGuest(scope.row)"
            >
              封停账号
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </el-dialog>
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
