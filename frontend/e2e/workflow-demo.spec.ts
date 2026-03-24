import { expect, test } from '@playwright/test';

const E2E_API = 'http://127.0.0.1:18080/api/v1';

test.describe('Reference minimal-demo workflow', () => {
	test('connect mock, load job, run, mock returns to loaded when list ends', async ({ page }) => {
		await page.request.post(`${E2E_API}/rtc/disconnect`);

		await page.goto('/workflow');

		await expect(page.getByRole('tablist', { name: 'Workflow steps' })).toBeVisible();

		await page.getByRole('button', { name: 'RTC session' }).click();
		// Reused e2e backend may still hold a session from a prior run.
		const disconnectBtn = page.getByTestId('disconnect-rtc');
		if (await disconnectBtn.isEnabled()) {
			const disconnectWait = page.waitForResponse(
				(r) => r.url().includes('/api/v1/rtc/disconnect') && r.status() === 204
			);
			await disconnectBtn.click();
			await disconnectWait;
		}

		const connectWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/rtc/connect') && r.status() === 204
		);
		await page.getByTestId('connect-mock').click();
		await connectWait;
		await expect(page.getByTestId('disconnect-rtc')).toBeEnabled();
		// Connect stays enabled so users can reconnect; only disabled while execution is running.
		await expect(page.getByTestId('connect-step-state')).toContainText('connected');

		await page.getByRole('button', { name: 'Job parameters' }).click();
		await page.getByTestId('param-label').fill('e2e-demo');

		await page.getByRole('button', { name: 'Load job' }).click();
		const loadWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/minimal-demo') && r.request().method() === 'POST' && r.status() === 200
		);
		await page.getByTestId('load-job').click();
		await loadWait;
		await expect(page.getByTestId('job-id')).toContainText('Job id:');

		await page.getByRole('button', { name: 'Run control' }).click();
		const runWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/jobs/minimal-demo/run') && r.status() === 204
		);
		await page.getByTestId('start-run').click();
		await runWait;
		await expect(page.getByTestId('start-run')).toBeEnabled();
		await expect(page.getByTestId('stop-run')).toBeDisabled();

		await page.getByRole('button', { name: 'RTC status' }).click();
		await page.getByTestId('refresh-status').click();
		await expect(page.getByTestId('connection-state')).toContainText('loaded');
	});

	test('RTC window shows health JSON', async ({ page }) => {
		await page.goto('/rtc');
		await page.getByTestId('rtc-refresh').click();
		await expect(page.getByTestId('rtc-page-health')).toContainText('"status"');
	});
});
