<script setup lang="ts">
import { reactive, ref } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import axios from 'axios'

const router = useRouter()
// 移除硬编码端口，通过 Vite 代理走相对路径
const SERVER_URL = '/api'

const activeTab = ref('login')
const loading = ref(false)
const codeLoading = ref(false)
const countdown = ref(0)

const loginForm = reactive({
  email: '',
  password: ''
})

const registerForm = reactive({
  email: '',
  code: '',
  password: ''
})

// 处理发送验证码
const handleSendCode = async () => {
  if (!registerForm.email) {
    ElMessage.warning('请输入注册邮箱')
    return
  }
  codeLoading.value = true
  try {
    const response = await axios.post(`${SERVER_URL}/auth/send-code`, { email: registerForm.email })
    if (response.data.code === 20000) {
      ElMessage.success('验证码已发送至您的邮箱，5分钟内有效')
      countdown.value = 60
      const timer = setInterval(() => {
        countdown.value--
        if (countdown.value <= 0) {
          clearInterval(timer)
        }
      }, 1000)
    } else {
      ElMessage.error(response.data.message || '发送失败')
    }
  } catch (error: any) {
    console.error(error)
    ElMessage.error(error.response?.data?.message || '网络请求失败，请检查 C++ 后端是否启动')
  } finally {
    codeLoading.value = false
  }
}

// 处理注册请求
const handleRegister = async () => {
  if (!registerForm.email || !registerForm.code || !registerForm.password) {
    ElMessage.warning('请完整填写注册信息')
    return
  }
  loading.value = true
  try {
    const response = await axios.post(`${SERVER_URL}/auth/register`, registerForm)
    if (response.data.code === 20000) {
      ElMessage.success('注册成功，请登录')
      activeTab.value = 'login'
      loginForm.email = registerForm.email
    } else {
      ElMessage.error(response.data.message || '注册失败')
    }
  } catch (error: any) {
    console.error(error)
    ElMessage.error(error.response?.data?.message || '网络请求失败')
  } finally {
    loading.value = false
  }
}

// 处理登录请求
const handleLogin = async () => {
  if (!loginForm.email || !loginForm.password) {
    ElMessage.warning('请输入邮箱和密码')
    return
  }
  loading.value = true
  try {
    const response = await axios.post(`${SERVER_URL}/auth/login`, loginForm)
    if (response.data.code === 20000) {
      ElMessage.success('登录成功')
      localStorage.setItem('token', response.data.data.token)
      localStorage.setItem('email', response.data.data.email)
      localStorage.setItem('role', response.data.data.role.toString())
      localStorage.setItem('id', response.data.data.id)
      router.push('/')
    } else {
      ElMessage.error(response.data.message || '登录失败')
    }
  } catch (error: any) {
    console.error(error)
    if (error.response && error.response.status === 401) {
      ElMessage.error('邮箱或密码错误')
    } else if (error.response && error.response.data?.message) {
      ElMessage.error(error.response.data.message)
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
        <h2>Audio Radar 访问控制</h2>
      </template>

      <el-tabs v-model="activeTab" stretch>
        <el-tab-pane label="登录" name="login">
          <el-form :model="loginForm" label-width="60px" @keyup.enter="handleLogin" class="mt-4">
            <el-form-item label="邮箱">
              <el-input v-model="loginForm.email" placeholder="请输入注册邮箱" />
            </el-form-item>
            <el-form-item label="密码">
              <el-input v-model="loginForm.password" type="password" placeholder="请输入密码" show-password />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" :loading="loading" @click="handleLogin" style="width: 100%">登录</el-button>
            </el-form-item>
          </el-form>
        </el-tab-pane>

        <el-tab-pane label="注册" name="register">
          <el-form :model="registerForm" label-width="60px" @keyup.enter="handleRegister" class="mt-4">
            <el-form-item label="邮箱">
              <el-input v-model="registerForm.email" placeholder="请输入注册邮箱" />
            </el-form-item>
            <el-form-item label="验证码">
              <div style="display: flex; gap: 10px; width: 100%">
                <el-input v-model="registerForm.code" placeholder="6位验证码" style="flex: 1" />
                <el-button type="primary" :loading="codeLoading" :disabled="countdown > 0 || !registerForm.email" @click="handleSendCode">
                  {{ countdown > 0 ? `${countdown}秒后重试` : '获取验证码' }}
                </el-button>
              </div>
            </el-form-item>
            <el-form-item label="密码">
              <el-input v-model="registerForm.password" type="password" placeholder="请设置您的密码" show-password />
            </el-form-item>
            <el-form-item>
              <el-button type="success" :loading="loading" @click="handleRegister" style="width: 100%">注册</el-button>
            </el-form-item>
          </el-form>
        </el-tab-pane>
      </el-tabs>
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
  width: 420px;
}
h2 {
  text-align: center;
  margin: 0;
  color: #303133;
}
.mt-4 {
  margin-top: 16px;
}
</style>
