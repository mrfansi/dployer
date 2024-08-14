#!/bin/bash

chmod -R 777 /app/bootstrap/cache
chmod -R 777 /app/storage

# Function to check for "prod" or "build" script in package.json and run the appropriate command
run_build_or_prod() {
  if [ -f "/app/package.json" ]; then
    # Check if "prod" script exists
    if grep -q '"prod":' /app/package.json; then
      if [ -f "/app/package-lock.json" ]; then
        su application -c "/usr/local/bin/npm run prod --prefix /app" || true
      elif [ -f "/app/yarn.lock" ]; then
        su application -c "/usr/local/bin/yarn prod --cwd /app" || true
      fi
    # Check if "build" script exists
    elif grep -q '"build":' /app/package.json; then
      if [ -f "/app/package-lock.json" ]; then
        su application -c "/usr/local/bin/npm run build --prefix /app" || true
      elif [ -f "/app/yarn.lock" ]; then
        su application -c "/usr/local/bin/yarn build --cwd /app" || true
      fi
    fi
  fi
}

# Check if composer.json exists, then update and install PHP dependencies
if [ -f "/app/composer.json" ]; then
  su application -c "/usr/local/bin/composer update --working-dir=/app"
  su application -c "/usr/local/bin/composer install --working-dir=/app"
fi

# Check if package-lock.json or yarn.lock exists, then install Node.js dependencies
if [ -f "/app/package-lock.json" ]; then
  su application -c "/usr/local/bin/npm install --prefix /app"
elif [ -f "/app/yarn.lock" ]; then
  su application -c "/usr/local/bin/yarn --cwd /app"
fi

# Set ownership of the vendor and node_modules directories to the application user
chown -R application:application /app/vendor || true
chown -R application:application /app/node_modules || true

# Run the build or prod script if found, and skip errors
run_build_or_prod

# Optimize the Laravel application
su application -c "/usr/local/bin/php /app/artisan optimize"