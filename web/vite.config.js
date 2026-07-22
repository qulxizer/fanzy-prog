import { defineConfig } from 'vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

export default defineConfig({
    plugins: [viteSingleFile()],
    server: {
        proxy: {
            '/config': 'http://127.0.0.1:8080',
        },
    },
});
