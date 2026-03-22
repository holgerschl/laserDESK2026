import { expect, test } from '@playwright/test';

/**
 * Base URL for the backend started by `playwright.config.ts` (e2e-backend.mjs on port 18080).
 * The dev server proxies /api to the same host via LASERDESK_BACKEND_URL.
 */
const E2E_API_BASE = 'http://127.0.0.1:18080/api/v1';

test.describe('Backend server connection', () => {
	test('GET /health responds', async ({ request }) => {
		const res = await request.get(`${E2E_API_BASE}/health`);
		expect(res.status()).toBe(200);
		const body = (await res.json()) as { status?: string };
		expect(body.status).toBeTruthy();
	});

	test('POST /rtc/connect (mock) returns 204 and status shows connected', async ({ request }) => {
		await request.post(`${E2E_API_BASE}/rtc/disconnect`);

		const connect = await request.post(`${E2E_API_BASE}/rtc/connect`, {
			headers: { 'Content-Type': 'application/json' },
			data: { mode: 'mock' }
		});
		expect(connect.status()).toBe(204);

		const st = await request.get(`${E2E_API_BASE}/rtc/status`);
		expect(st.status()).toBe(200);
		const json = (await st.json()) as { connection_state?: string };
		expect(json.connection_state).toMatch(/connected/);
	});

	test('workflow Connect (mock) reaches backend and updates UI', async ({ page }) => {
		await page.goto('/workflow');
		await expect(page.getByRole('tablist', { name: 'Workflow steps' })).toBeVisible();

		await page.getByRole('button', { name: 'RTC session' }).click();

		const disconnectBtn = page.getByTestId('disconnect-rtc');
		if (await disconnectBtn.isEnabled()) {
			const disconnectWait = page.waitForResponse(
				(r) => r.url().includes('/api/v1/rtc/disconnect') && r.status() === 204
			);
			await disconnectBtn.click();
			await disconnectWait;
		}

		const connectWait = page.waitForResponse(
			(r) =>
				r.url().includes('/api/v1/rtc/connect') &&
				r.request().method() === 'POST' &&
				r.status() === 204
		);
		await page.getByTestId('connect-mock').click();
		await connectWait;

		await expect(page.getByTestId('disconnect-rtc')).toBeEnabled();
		await expect(page.getByTestId('connect-step-state')).toContainText('connected');
		await expect(page.getByTestId('workflow-status-log-body')).toContainText('Connected (mock)');
	});
});
