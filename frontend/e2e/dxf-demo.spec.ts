import { expect, test } from '@playwright/test';

/** Same backend as playwright.config webServer (e2e-backend.mjs on 18080). */
const E2E_API = 'http://127.0.0.1:18080/api/v1';

test.describe('DXF Phase G demo', () => {
	test('load demo from static, parse, load RTC, run', async ({ page }) => {
		// RTC session is one per backend process; reset so we never rely on UI race (disconnect disabled before first refresh).
		const disc = await page.request.post(`${E2E_API}/rtc/disconnect`);
		expect(disc.status(), 'API disconnect should succeed').toBe(204);

		await page.goto('/dxf', { waitUntil: 'load' });
		// /dxf mount runs refreshRtc(); wait until status is shown (not "—") so Connect is reliable in CI.
		await expect(page.getByTestId('dxf-rtc-state')).toContainText('disconnected', { timeout: 30_000 });

		const connectRespPromise = page.waitForResponse((r) => {
			const req = r.request();
			return req.url().includes('/api/v1/rtc/connect') && req.method() === 'POST';
		});
		await page.getByTestId('dxf-connect-mock').click();
		const connectResp = await connectRespPromise;
		expect(
			connectResp.status(),
			`POST /rtc/connect expected 204, got body: ${(await connectResp.text()).slice(0, 500)}`
		).toBe(204);
		await expect(page.getByTestId('dxf-disconnect')).toBeEnabled();

		const parseWait = page.waitForResponse(
			(r) =>
				r.url().includes('/api/v1/jobs/dxf') &&
				r.request().method() === 'POST' &&
				!r.url().includes('/load') &&
				r.status() === 200
		);
		await page.getByTestId('dxf-load-demo-static').click();
		await parseWait;

		await expect(page.getByTestId('dxf-entity-count')).toContainText('LINE entities');
		await expect(page.getByTestId('dxf-entity-table').locator('tbody tr').first()).toBeVisible();

		const loadRtcWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/dxf/') && r.url().endsWith('/load') && r.status() === 204
		);
		await page.getByTestId('dxf-load-rtc').click();
		await loadRtcWait;

		await expect(page.getByTestId('dxf-start')).toBeEnabled({ timeout: 20_000 });

		const runWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/dxf/') && r.url().endsWith('/run') && r.status() === 204
		);
		await page.getByTestId('dxf-start').click();
		await runWait;

		await expect(page.getByTestId('dxf-rtc-state')).toContainText('running');

		await expect(page.getByTestId('dxf-stop')).toBeEnabled({ timeout: 15_000 });

		const stopWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/dxf/') && r.url().endsWith('/stop') && r.status() === 204
		);
		await page.getByTestId('dxf-stop').click();
		await stopWait;
		await expect(page.getByTestId('dxf-rtc-state')).toContainText('loaded');
	});
});
