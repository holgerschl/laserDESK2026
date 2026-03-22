import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig, loadEnv } from 'vite';

export default defineConfig(({ mode }) => {
	const env = loadEnv(mode, process.cwd(), '');
	const target = env.LASERDESK_BACKEND_URL || 'http://127.0.0.1:8080';
	return {
		plugins: [sveltekit()],
		server: {
			proxy: {
				'/api': { target, changeOrigin: true }
			}
		},
		preview: {
			proxy: {
				'/api': { target, changeOrigin: true }
			}
		}
	};
});
