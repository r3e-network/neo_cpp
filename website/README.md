# Neo C++ Website

This is the official website for Neo C++ - a production-ready Neo blockchain node implementation in modern C++20.

## 🌟 Features

- **Modern Design**: Clean, responsive design optimized for all devices
- **Performance Optimized**: Fast loading with optimized assets and caching
- **SEO Ready**: Comprehensive meta tags, structured data, and sitemap
- **Security Hardened**: Security headers, CSP, and best practices implemented
- **Accessibility**: WCAG 2.1 AA compliant with semantic HTML and ARIA labels

## 🚀 Deployment

This website is designed for deployment on Netlify with the following features:

### Netlify Configuration
- **Build Command**: Static site deployment (no build process required)
- **Publish Directory**: Root directory
- **Headers**: Security headers for enhanced protection
- **Redirects**: SEO-friendly redirects and shortcuts
- **Performance**: Asset optimization and caching rules

### Security Features
- Content Security Policy (CSP)
- HTTP Strict Transport Security (HSTS)
- X-Frame-Options, X-Content-Type-Options
- Referrer Policy and Permissions Policy

### Performance Optimizations
- Static asset caching (CSS, JS, images)
- Gzip compression
- Image optimization
- Minified CSS and JavaScript

## 📁 Structure

```
website/
├── index.html              # Main homepage
├── css/
│   └── style.css          # Main stylesheet
├── js/
│   └── script.js          # Interactive functionality
├── docs/
│   └── getting-started.html # Documentation pages
├── images/                # Static assets
├── netlify.toml          # Netlify configuration
├── _headers              # HTTP headers
├── _redirects            # URL redirects
├── robots.txt            # Search engine crawling rules
├── sitemap.xml           # Site structure for SEO
└── security.txt          # Security contact information
```

## 🛠️ Local Development

To run the website locally:

1. Clone the repository
2. Navigate to the website directory
3. Start a local server:

```bash
# Using Python
python -m http.server 8000

# Using Node.js
npx serve .

# Using PHP
php -S localhost:8000
```

4. Open http://localhost:8000 in your browser

## 📱 Responsive Design

The website is fully responsive and tested on:
- Mobile devices (320px+)
- Tablets (768px+)
- Desktop computers (1024px+)
- Large screens (1440px+)

## 🎨 Design System

### Colors
- **Primary**: #00d4aa (Neo Green)
- **Secondary**: #6c5ce7 (Purple)
- **Dark**: #0a0e27 (Near Black)
- **Light**: #f8f9fa (Off White)

### Typography
- **Primary Font**: Inter (Google Fonts)
- **Monospace**: SF Mono, Monaco, Source Code Pro

### Components
- Navigation with mobile hamburger menu
- Hero section with animated code terminal
- Feature cards with hover effects
- Tabbed installation instructions
- Documentation layout
- Footer with social links

## 🔍 SEO Features

- Semantic HTML structure
- Meta tags for social media (Open Graph, Twitter Cards)
- JSON-LD structured data
- Sitemap.xml for search engines
- Robots.txt for crawler instructions
- Clean, descriptive URLs

## 📊 Analytics

The website is prepared for analytics integration:
- Google Analytics 4 ready
- Custom event tracking
- Performance monitoring
- User interaction tracking

## 🚀 Performance

- Lighthouse Score: 90+ (Performance, Accessibility, Best Practices, SEO)
- Page Load Time: <2 seconds
- First Contentful Paint: <1.5 seconds
- Cumulative Layout Shift: <0.1

## 🔒 Security

- Content Security Policy implemented
- All external resources from trusted CDNs
- No inline scripts (except for analytics)
- Security headers via Netlify configuration
- Regular dependency updates

## 📧 Contact

For website-related issues:
- GitHub Issues: [Website Issues](https://github.com/r3e-network/neo_cpp/issues)
- General Contact: contact@neo-cpp.org

## 📄 License

This website is part of the Neo C++ project and is licensed under the MIT License.

---

Built with ❤️ for the Neo blockchain community