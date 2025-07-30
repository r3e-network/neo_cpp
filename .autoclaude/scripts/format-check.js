#!/usr/bin/env node
const { execSync } = require('child_process');
const path = require('path');

try {
  const scriptPath = path.join(__dirname, 'format-check.sh');
  const result = execSync(scriptPath, { encoding: 'utf8', stdio: 'pipe' });
  console.log(result);
  process.exit(0);
} catch (error) {
  console.error(error.stdout || error.message);
  process.exit(error.status || 1);
}