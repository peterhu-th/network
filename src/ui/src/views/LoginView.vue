<script setup lang="ts">
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import axios from 'axios'

const router = useRouter()
const SERVER_URL = `http://${window.location.hostname}:8080`

const loginForm = reactive({
  username: '',
  password: ''
})

const loading = ref(false)

// 处理登录请求 (携带用户密码向后端换取 JWT)
const handleLogin = async () => {
  if (!loginForm.username || !loginForm.password) {
    ElMessage.warning('请输入用户名和密码')
    return
  }
  loading.value = true
  try {
    const response = await axios.post(`${SERVER_URL}/login`, loginForm)
    if (response.data.code === 20000) {
      ElMessage.success('登录成功')
      localStorage.setItem('token', response.data.data.token)
      localStorage.setItem('username', response.data.data.username)
      router.push('/')
    } else {
      ElMessage.error(response.data.message || '登录失败')
    }
  } catch (error: any) {
    console.error(error)
    if (error.response && error.response.status === 401) {
      ElMessage.error('用户名或密码错误')
    } else {
      ElMessage.error('网络请求失败，请检查 C++ 后端是否启动')
    }
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <div class="login-container">
    <el-card class="login-card">
      <template #header>
        <h2>系统登录</h2>
      </template>
      <el-form :model="loginForm" label-width="80px" @keyup.enter="handleLogin">
        <el-form-item label="用户名">
          <el-input v-model="loginForm.username" placeholder="请输入用户名 (如 admin)" />
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="loginForm.password" type="password" placeholder="请输入密码" show-password />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" :loading="loading" @click="handleLogin" style="width: 100%">登录</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<style scoped>
.login-container {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100vh;
  background-color: #f5f7fa;
}
.login-card {
  width: 400px;
}
h2 {
  text-align: center;
  margin: 0;
  color: #303133;
}
</style>
