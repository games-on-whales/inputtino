import {defineConfig, loadEnv} from 'vite'
import {svelte} from '@sveltejs/vite-plugin-svelte'

// https://vitejs.dev/config/
export default defineConfig(({command, mode}) => {
    // Load env file based on `mode` in the current working directory.
    // Set the third parameter to '' to load all env regardless of the `VITE_` prefix.
    const env = loadEnv(mode, process.cwd(), '')
    return {
        plugins: [svelte()],
        server: {
            cors: false,
            proxy: {
                '/api': {
                    "target": env.INPUTTINO_SERVER_URL,
                    secure: false,
                    changeOrigin: true,
                }
            }
        }
    }
})
