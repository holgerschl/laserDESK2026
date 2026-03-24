import { expect, test } from '@playwright/test';

const E2E_API = 'http://127.0.0.1:18080/api/v1';

test.describe('Scene editor Phase H', () => {
	test('submit scene, load RTC, run, stop', async ({ page }) => {
		const disc = await page.request.post(`${E2E_API}/rtc/disconnect`);
		expect(disc.status(), 'API disconnect should succeed').toBe(204);

		await page.goto('/editor', { waitUntil: 'load' });
		await expect(page.getByTestId('editor-rtc-state')).toContainText('disconnected', { timeout: 30_000 });

		const connectRespPromise = page.waitForResponse((r) => {
			const req = r.request();
			return req.url().includes('/api/v1/rtc/connect') && req.method() === 'POST';
		});
		await page.getByTestId('connect-mock').click();
		const connectResp = await connectRespPromise;
		expect(connectResp.status(), 'POST /rtc/connect expected 204').toBe(204);
		await expect(page.getByTestId('disconnect-rtc')).toBeEnabled();

		const sceneWait = page.waitForResponse(
			(r) =>
				r.url().includes('/api/v1/jobs/scene') &&
				r.request().method() === 'POST' &&
				r.status() === 200
		);
		await page.getByTestId('editor-submit-scene').click();
		await sceneWait;

		await expect(page.getByTestId('editor-entity-count')).toContainText('LINE entities');
		await expect(page.getByTestId('editor-entity-table').locator('tbody tr').first()).toBeVisible();

		const loadRtcWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/dxf/') && r.url().endsWith('/load') && r.status() === 204
		);
		await page.getByTestId('editor-load-rtc').click();
		await loadRtcWait;

		await expect(page.getByTestId('editor-start')).toBeEnabled({ timeout: 20_000 });

		const runWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/dxf/') && r.url().endsWith('/run') && r.status() === 204
		);
		await page.getByTestId('editor-start').click();
		await runWait;

		// Mock RTC simulates immediate end-of-list: back to loaded (Start on, Stop off).
		await expect(page.getByTestId('editor-rtc-state')).toContainText('loaded');
		await expect(page.getByTestId('editor-start')).toBeEnabled();
		await expect(page.getByTestId('editor-stop')).toBeDisabled();
	});
});
