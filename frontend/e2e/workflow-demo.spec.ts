import { expect, test } from '@playwright/test';

test.describe('Reference minimal-demo workflow', () => {
	test('connect mock, load job, run, see running state', async ({ page }) => {
		await page.goto('/');

		await expect(page.getByRole('tablist', { name: 'Workflow steps' })).toBeVisible();

		await page.getByRole('button', { name: 'RTC session' }).click();
		const connectWait = page.waitForResponse(
			(r) => r.url().includes('/api/v1/rtc/connect') && r.status() === 204
		);
		await page.getByTestId('connect-mock').click();
		await connectWait;

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

		await page.getByRole('button', { name: 'RTC status' }).click();
		await page.getByTestId('refresh-status').click();
		await expect(page.getByTestId('connection-state')).toContainText('running');
	});

	test('RTC window shows health JSON', async ({ page }) => {
		await page.goto('/rtc');
		await page.getByTestId('rtc-refresh').click();
		await expect(page.getByTestId('rtc-page-health')).toContainText('"status"');
	});
});
