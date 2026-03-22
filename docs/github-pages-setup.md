# GitHub Pages setup (fix 404 / failing “Deploy GitHub Pages”)

If **`https://holgerschl.github.io/laserDESK2026/`** shows **404** or Actions **Deploy GitHub Pages** is **red**, walk through this once.

## 1. Turn Pages on and choose GitHub Actions

1. Open: `https://github.com/holgerschl/laserDESK2026/settings/pages`
2. Under **Build and deployment**:
   - **Source:** select **GitHub Actions** (not “Deploy from a branch”).
3. Save if prompted.

Until this is set, **`configure-pages`** in the workflow often fails quickly and the site will not exist.

## 2. Approve the `github-pages` environment (if required)

1. Open: `https://github.com/holgerschl/laserDESK2026/settings/environments`
2. Click **github-pages**.
3. If **Required reviewers** is enabled, open the workflow run on **Actions** and use **Review deployments** → **Approve** when a run waits.

## 3. See the exact error

1. Open: `https://github.com/holgerschl/laserDESK2026/actions/workflows/deploy-github-pages.yml`
2. Click the latest run (red).
3. Open the job **github-pages** (single job after workflow update).
4. Expand the first **red** step and read the log (often **Configure GitHub Pages**, **npm ci**, or **Deploy**).

## 4. Re-run after fixing settings

On the workflow page, use **Run workflow** → branch **main**, or push any commit to **main**.

## 5. When it works

**Settings → Pages** will show **“Your site is live at …”**. Allow **1–2 minutes** for the CDN after a green run.

---

*Repository: [holgerschl/laserDESK2026](https://github.com/holgerschl/laserDESK2026)*
