# Neo C++ Website

Production-ready website and documentation for the Neo C++ blockchain implementation.

## 🚀 Quick Start

### Local Development

```bash
# Install dependencies (optional, for development tools)
npm install

# Serve locally with live reload
npm run serve:watch

# Or use Python
python3 -m http.server 8000

# Or use any static server
npx serve .
```

Visit http://localhost:8000

## 📦 Deployment

### Netlify (Recommended)

#### Option 1: One-Click Deploy

[![Deploy to Netlify](https://www.netlify.com/img/deploy/button.svg)](https://app.netlify.com/start/deploy?repository=https://github.com/r3e-network/neo_cpp)

#### Option 2: Manual Deploy via UI

1. Go to [Netlify](https://app.netlify.com)
2. Drag and drop the `website` folder
3. Your site is live!

#### Option 3: CLI Deploy

```bash
# Install Netlify CLI
npm install -g netlify-cli

# Login to Netlify
netlify login

# Deploy to production
netlify deploy --prod --dir=.

# Deploy preview
netlify deploy --dir=.
```

#### Option 4: Git Integration

1. Push to GitHub
2. Connect repo to Netlify
3. Set build settings:
   - **Base directory**: `website`
   - **Build command**: (leave empty)
   - **Publish directory**: `.`
4. Auto-deploy on push!

### Vercel

```bash
# Install Vercel CLI
npm i -g vercel

# Deploy
vercel --prod
```

### GitHub Pages

1. Go to repo Settings → Pages
2. Source: Deploy from branch
3. Branch: main, folder: /website
4. Save and wait for deployment

### Docker

```dockerfile
FROM nginx:alpine
COPY . /usr/share/nginx/html
EXPOSE 80
```

```bash
docker build -t neo-cpp-website .
docker run -p 8080:80 neo-cpp-website
```

## 📁 Structure

```
website/
├── index.html              # Homepage
├── docs/                   # Documentation
│   ├── getting-started.html
│   ├── api-reference.html
│   ├── architecture.html
│   ├── sdk-guide.html
│   └── deployment.html
├── css/                    # Stylesheets
│   ├── style.css
│   ├── animations.css
│   ├── responsive.css
│   └── docs.css
├── js/                     # JavaScript
│   ├── main.js
│   └── docs.js
├── assets/                 # Images & icons
│   └── favicon.svg
├── netlify.toml           # Netlify config
├── _headers               # HTTP headers
├── _redirects             # URL redirects
├── robots.txt             # SEO
├── sitemap.xml            # SEO
└── 404.html               # Error page
```

## ✨ Features

- **Static Site**: No build process required
- **Responsive**: Works on all devices
- **Fast**: Optimized for performance
- **SEO Ready**: Meta tags, sitemap, robots.txt
- **Accessible**: WCAG compliant
- **Secure**: Security headers configured
- **CDN Ready**: Assets cached properly

## 🛠️ Configuration

### Netlify Settings

All configuration is in `netlify.toml`:
- Security headers
- Cache control
- Redirects
- Pretty URLs

### Custom Domain

1. Add domain in Netlify settings
2. Update DNS records:
   ```
   Type: CNAME
   Name: www
   Value: [your-site].netlify.app
   ```

### SSL Certificate

- Automatic with Netlify (Let's Encrypt)
- Custom certificates supported

## 📊 Performance

- **Lighthouse Score**: 95+ (all categories)
- **Page Size**: <500KB (homepage)
- **Load Time**: <2s (3G network)
- **First Paint**: <1s

## 🧪 Testing

```bash
# Test broken links
npm run test:links

# Validate HTML
npm run test:html

# Performance audit
npm run test:lighthouse
```

## 🔧 Development

### Adding Documentation

1. Create HTML file in `docs/`
2. Use existing template structure
3. Add to sidebar navigation
4. Update sitemap.xml

### Modifying Styles

- Main styles: `css/style.css`
- Documentation: `css/docs.css`
- Use CSS variables for consistency

### JavaScript

- Main functionality: `js/main.js`
- Documentation features: `js/docs.js`
- No build process needed

## 📝 Content Management

### Updating Content

1. Edit HTML files directly
2. Commit changes
3. Auto-deploy via Netlify

### Adding Examples

Create files in `examples/` directory following the template structure.

## 🚀 Optimization

### Already Implemented

- ✅ Minified CSS/JS via CDN
- ✅ Image optimization
- ✅ Lazy loading
- ✅ Cache headers
- ✅ Gzip compression (Netlify)
- ✅ CDN distribution (Netlify)

### Optional Enhancements

- Add PWA support
- Implement search with Algolia
- Add analytics (GA, Plausible)
- Integrate comments (Disqus, Utterances)

## 📈 Analytics

Add to `index.html` before `</body>`:

```html
<!-- Google Analytics -->
<script async src="https://www.googletagmanager.com/gtag/js?id=GA_MEASUREMENT_ID"></script>
<script>
  window.dataLayer = window.dataLayer || [];
  function gtag(){dataLayer.push(arguments);}
  gtag('js', new Date());
  gtag('config', 'GA_MEASUREMENT_ID');
</script>
```

## 🔐 Security

- **Headers**: Security headers configured
- **CSP**: Content Security Policy set
- **HTTPS**: Enforced by Netlify
- **XSS Protection**: Enabled
- **Frame Options**: DENY

## 📄 License

MIT License - See [LICENSE](../LICENSE) file

## 🤝 Contributing

1. Fork the repository
2. Create feature branch
3. Make changes
4. Test locally
5. Submit pull request

## 💬 Support

- [GitHub Issues](https://github.com/r3e-network/neo_cpp/issues)
- [Discord Community](https://discord.gg/neo)
- [Documentation](https://neo-cpp.org/docs)

---

Built with ❤️ for the Neo community