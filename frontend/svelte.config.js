import adapter from '@sveltejs/adapter-static';

/** Base path for GitHub project Pages, e.g. `/laserDESK2026`. Set at build time only. */
const base = process.env.SVELTEKIT_BASE_PATH || '';

/** @type {import('@sveltejs/kit').Config} */
const config = {
	kit: {
		adapter: adapter({ strict: true }),
		paths: { base }
	},
	vitePlugin: {
		dynamicCompileOptions: ({ filename }) =>
			filename.includes('node_modules') ? undefined : { runes: true }
	}
};

export default config;
