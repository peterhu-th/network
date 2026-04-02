import { createApp } from 'vue'
import { createPinia } from 'pinia'

// --- 引入 Element Plus 及其样式和图标 ---
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import * as ElementPlusIconsVue from '@element-plus/icons-vue'
// ----------------------------------------

import App from './App.vue'
import router from './router'

const app = createApp(App)

app.use(createPinia())
app.use(router)

// --- 将 Element Plus 挂载到全局 ---
app.use(ElementPlus)
for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
    app.component(key, component)
}
// ----------------------------------------

app.mount('#app')