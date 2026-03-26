#!/usr/bin/env python3
"""
GitHub Pages 模拟测试脚本

这个脚本模拟 GitHub Pages 的部署和访问行为，帮助检测潜在的链接问题。

用法:
  python3 scripts/test_gh_pages_deployment.py
"""

import os
import re
import json
from pathlib import Path
from urllib.parse import urljoin, urlparse
from collections import defaultdict

# ANSI color codes
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'

def print_success(msg):
    print(f"{Colors.GREEN}✅ {msg}{Colors.RESET}")

def print_error(msg):
    print(f"{Colors.RED}❌ {msg}{Colors.RESET}")

def print_warning(msg):
    print(f"{Colors.YELLOW}⚠️  {msg}{Colors.RESET}")

def print_info(msg):
    print(f"{Colors.BLUE}ℹ️  {msg}{Colors.RESET}")

def main():
    print("=" * 70)
    print("GitHub Pages Deployment Simulator")
    print("=" * 70)
    print()

    # Configuration
    base_url = "https://adam-ikari.github.io/uvhttp/"
    dist_path = Path("docs/.vitepress/dist")

    if not dist_path.exists():
        print_error("dist directory not found!")
        print("Please run: cd docs && DEPLOY=gh-pages npm run build")
        return 1

    # Check 1: .nojekyll file (CRITICAL)
    print_info("Check 1: .nojekyll file")
    nojekyll_file = dist_path / ".nojekyll"
    if nojekyll_file.exists():
        print_success(".nojekyll file exists (Jekyll will be disabled)")
    else:
        print_error(".nojekyll file MISSING (CRITICAL!)")
        print("  This will cause 404s for VitePress assets!")
        print()

    # Check 2: Base path configuration
    print_info("Check 2: Base path configuration")
    index_html = dist_path / "index.html"
    if not index_html.exists():
        print_error("index.html not found!")
        return 1

    with open(index_html, encoding='utf-8') as f:
        index_content = f.read()

    issues = []
    warnings = []

    # Check for base path in asset references
    if "/uvhttp/assets/" in index_content:
        print_success("Assets have correct /uvhttp/ base path")
    else:
        print_error("Assets missing /uvhttp/ base path")
        issues.append("Asset paths don't include base path")

    # Check for old favicon path
    if 'href="/favicon.svg"' in index_content or 'src="/favicon.svg"' in index_content:
        print_warning("Favicon referenced without base path (will 404)")
        warnings.append("Root assets might fail without base path")
    elif "/uvhttp/favicon.svg" in index_content:
        print_success("Favicon has correct /uvhttp/ base path")

    # Check 3: CSP configuration
    print_info("Check 3: Content Security Policy")
    if "Content-Security-Policy" in index_content:
        csp_match = re.search(r'Content-Security-Policy.*?content="([^"]+)"', index_content, re.IGNORECASE)
        if csp_match:
            csp = csp_match.group(1)
            csp_issues = []
            
            required_directives = {
                'script-src': ['unsafe-eval', 'unsafe-inline'],
                'connect-src': ["'self'"],
            }
            
            for directive, required_values in required_directives.items():
                if directive in csp:
                    for value in required_values:
                        if value in csp:
                            print_success(f"CSP includes {directive} {value}")
                        else:
                            csp_issues.append(f"{directive} missing {value}")
                            print_warning(f"CSP {directive} missing {value}")
                else:
                    csp_issues.append(f"Missing {directive}")
                    print_warning(f"CSP missing {directive}")

            if not csp_issues:
                print_success("CSP configuration looks good")
    else:
        print_warning("No CSP policy (not critical for testing)")

    print()

    # Check 4: Navigation links
    print_info("Check 4: Navigation links")
    nav_links = {
        "/uvhttp/": "Home",
        "/uvhttp/guide/getting-started.html": "Guide",
        "/uvhttp/api/introduction.html": "API",
        "/uvhttp/guide/performance.html": "Performance",
        "/uvhttp/guide/versions.html": "Versions",
        "/uvhttp/zh/": "Chinese version",
    }

    nav_issues = []
    for link, name in nav_links.items():
        if link in index_content:
            print_success(f"{name} link has correct base path")
        else:
            print_error(f"{name} link not found or missing base path")
            nav_issues.append(name)

    print()

    # Check 5: Asset files existence
    print_info("Check 5: Asset files")
    assets_dir = dist_path / "assets"
    if not assets_dir.exists():
        print_error("Assets directory not found!")
        issues.append("Assets directory missing")
        return 1

    # Find all asset references in index.html
    asset_refs = re.findall(r'href="(/uvhttp/assets/[^"]+)"', index_content)
    asset_refs.extend(re.findall(r'src="(/uvhttp/assets/[^"]+)"', index_content))
    
    asset_issues = 0
    checked_assets = set()
    for ref in list(set(asset_refs))[:10]:  # Check first 10 unique assets
        filename = ref.split('/')[-1]
        file_path = assets_dir / filename
        
        if file_path.exists():
            checked_assets.add(filename)
        else:
            print_error(f"Referenced asset not found: {filename}")
            asset_issues += 1

    if asset_issues == 0:
        print_success(f"All checked assets exist ({len(checked_assets)} verified)")

    print()

    # Check 6: HTML files
    print_info("Check 6: HTML files structure")
    html_files = list(dist_path.rglob("*.html"))
    print_success(f"Found {len(html_files)} HTML files")

    # Check for missing .html extensions in links (VitePress SPA routing)
    if index_html.exists():
        with open(index_html) as f:
            content = f.read()
        
        # Check for client-side routing (no .html extension in some links)
        if 'href="/uvhttp/"' in content:
            print_success("Root link uses / (SPA routing)")
    
    print()

    # Check 7: Verify critical files
    print_info("Check 7: Critical files existence")
    critical_files = [
        "index.html",
        "404.html",
        "favicon.svg",
        "robots.txt",
        "sitemap.xml",
    ]

    file_issues = 0
    for filename in critical_files:
        file_path = dist_path / filename
        if file_path.exists():
            print_success(f"{filename} exists")
        else:
            print_warning(f"{filename} not found (may not be critical)")
            file_issues += 1

    print()

    # Summary
    print("=" * 70)
    print("Summary")
    print("=" * 70)

    total_issues = len(issues) + len(nav_issues) + asset_issues + file_issues
    total_warnings = len(warnings)

    if total_issues == 0:
        print_success("All critical checks passed!")
    else:
        print_error(f"Found {total_issues} critical issues:")
        for issue in issues + nav_issues:
            print(f"  - {issue}")

    if total_warnings > 0:
        print_warning(f"Found {total_warnings} warnings:")
        for warning in warnings:
            print(f"  - {warning}")

    print()
    
    # GitHub Pages deployment simulation
    print_info("GitHub Pages deployment simulation")
    print()
    print("Expected URLs after deployment:")
    print(f"  🌐 Home:        {base_url}")
    print(f"  🌐 Guide:       {urljoin(base_url, 'guide/getting-started.html')}")
    print(f"  🌐 API:         {urljoin(base_url, 'api/introduction.html')}")
    print(f"  🌐 Performance:  {urljoin(base_url, 'guide/performance.html')}")
    print(f"  🌐 Chinese:     {urljoin(base_url, 'zh/')}")
    print()

    # Test recommendations
    print_info("Testing recommendations")
    print("After deployment to GitHub Pages:")
    print("  1. Open the site in a browser")
    print("  2. Check browser console for errors (F12)")
    print("  3. Click all navigation links")
    print("  4. Verify no 404 errors in Network tab")
    print("  5. Test Chinese version link")
    print("  6. Verify favicon displays")
    print()

    # Final verdict
    print("=" * 70)
    if total_issues == 0:
        print_success("✨ Site is ready for GitHub Pages deployment!")
        print()
        print("Next steps:")
        print("  1. Commit changes to repository")
        print("  2. Push to main or release/* branch")
        print("  3. Wait for CI/CD to complete")
        print("  4. Visit: " + base_url)
        return 0
    else:
        print_error("⚠️  Please fix the issues above before deploying")
        return 1

if __name__ == "__main__":
    exit(main())
