import { redirect } from '@sveltejs/kit';
import { base } from '$app/paths';

export const prerender = true;

/** Landing: Usage & installation first; workflow app lives under `/workflow`. */
export function load() {
	throw redirect(308, `${base}/usage`);
}
