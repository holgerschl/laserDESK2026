import { expect, test } from '@playwright/test';

test.describe('DXF Phase G demo', () => {
	test('load demo from static, parse, load RTC, run', async ({ page }) => {
		await page.goto('/dxf');

		const disconnectBtn = page.getByTestId('dxf-disconnect');
		if (await disconnectBtn.isEnabled()) {
			await disconnectBtn.click();
			await page.waitForResponse(
				(r) => r.url().includes('/api/v1/rtc/disconnect') && r.status() === 204
			);
		}

		const connectWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/rtc/connect') && r.status() === 204
		);
		await page.getByTestId('dxf-connect-mock').click();
		await connectWait;

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

		const runWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/dxf/') && r.url().endsWith('/run') && r.status() === 204
		);
		await page.getByTestId('dxf-start').click();
		await runWait;

		await expect(page.getByTestId('dxf-rtc-state')).toContainText('running');

		const stopWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/dxf/') && r.url().endsWith('/stop') && r.status() === 204
		);
		await page.getByTestId('dxf-stop').click();
		await stopWait;
		await expect(page.getByTestId('dxf-rtc-state')).toContainText('loaded');
	});
});
