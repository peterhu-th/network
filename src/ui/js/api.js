const BASE_URL = `http://${window.location.hostname}:8080/api`;
const AUTH_TOKEN = 'user';

export async function fetchAudioFiles(limit = 50, offset = 0) {
    try {
        const response = await fetch(`${BASE_URL}/files?limit=${limit}&offset=${offset}`, {
            method: 'GET',
            headers: {
                    'Authorization': `Bearer ${AUTH_TOKEN}`
                }
        });
        if (response.status === 401) {
            throw new Error("Authentication failed.");
        }
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        return await response.json();
    } catch (error) {
        console.error("Failed After Connection Server: ", error);
        return { statue: 'error', message: error.message };
    }
}

export function getDownloadUrl(id) {
    return `${BASE_URL}/download?id=${id}&token=${AUTH_TOKEN}`;
}

export async function downloadAudioFile(id, speed = 0) {
    let url = `${BASE_URL}/download?id=${id}`;
    if (speed > 0) url += `&speed=${speed}`;

    const response = await fetch(url, {
        method: 'GET',
        headers: { 'Authorization': `Bearer ${AUTH_TOKEN}` } // 将 Token 放在请求头
    });

    if (!response.ok) {
        throw new Error(`Download failed: ${response.status}`);
    }
    return await response.blob(); // 返回二进制大对象
}