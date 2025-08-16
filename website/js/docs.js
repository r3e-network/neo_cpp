/**
 * Neo C++ Documentation JavaScript
 * Handles documentation-specific functionality
 */

(function() {
    'use strict';

    // Sidebar Navigation
    function initSidebar() {
        const sidebarLinks = document.querySelectorAll('.sidebar-link');
        const sections = document.querySelectorAll('.docs-section');
        
        // Handle sidebar link clicks
        sidebarLinks.forEach(link => {
            link.addEventListener('click', function(e) {
                e.preventDefault();
                const targetId = this.getAttribute('href').substring(1);
                const targetSection = document.getElementById(targetId);
                
                if (targetSection) {
                    // Smooth scroll to section
                    targetSection.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                    
                    // Update active state
                    sidebarLinks.forEach(l => l.classList.remove('active'));
                    this.classList.add('active');
                    
                    // Update URL without page jump
                    history.pushState(null, null, '#' + targetId);
                }
            });
        });
        
        // Highlight active section on scroll
        function highlightActiveSection() {
            let currentSection = '';
            
            sections.forEach(section => {
                const rect = section.getBoundingClientRect();
                if (rect.top <= 100 && rect.bottom > 100) {
                    currentSection = section.id;
                }
            });
            
            if (currentSection) {
                sidebarLinks.forEach(link => {
                    link.classList.remove('active');
                    if (link.getAttribute('href') === '#' + currentSection) {
                        link.classList.add('active');
                    }
                });
            }
        }
        
        window.addEventListener('scroll', highlightActiveSection);
        highlightActiveSection(); // Initial check
    }

    // Installation Tabs
    function initInstallationTabs() {
        const tabButtons = document.querySelectorAll('.tab-btn');
        const tabContents = document.querySelectorAll('.tab-content');
        
        tabButtons.forEach(button => {
            button.addEventListener('click', function() {
                const targetTab = this.getAttribute('data-tab');
                
                // Update button states
                tabButtons.forEach(btn => btn.classList.remove('active'));
                this.classList.add('active');
                
                // Update content visibility
                tabContents.forEach(content => {
                    if (content.id === targetTab) {
                        content.classList.add('active');
                    } else {
                        content.classList.remove('active');
                    }
                });
            });
        });
    }

    // Copy Code Functionality
    function initCodeCopy() {
        const codeBlocks = document.querySelectorAll('pre code');
        
        codeBlocks.forEach(block => {
            const pre = block.parentElement;
            
            // Create copy button
            const copyButton = document.createElement('button');
            copyButton.className = 'copy-code-btn';
            copyButton.innerHTML = '<i class="fas fa-copy"></i>';
            copyButton.title = 'Copy code';
            
            // Position button
            pre.style.position = 'relative';
            pre.appendChild(copyButton);
            
            // Handle copy
            copyButton.addEventListener('click', async function() {
                const code = block.textContent;
                
                try {
                    await navigator.clipboard.writeText(code);
                    
                    // Show success feedback
                    this.innerHTML = '<i class="fas fa-check"></i>';
                    this.classList.add('copied');
                    
                    // Reset after 2 seconds
                    setTimeout(() => {
                        this.innerHTML = '<i class="fas fa-copy"></i>';
                        this.classList.remove('copied');
                    }, 2000);
                } catch (err) {
                    console.error('Failed to copy:', err);
                    
                    // Fallback for older browsers
                    const textarea = document.createElement('textarea');
                    textarea.value = code;
                    textarea.style.position = 'fixed';
                    textarea.style.opacity = '0';
                    document.body.appendChild(textarea);
                    textarea.select();
                    document.execCommand('copy');
                    document.body.removeChild(textarea);
                    
                    // Show feedback
                    this.innerHTML = '<i class="fas fa-check"></i>';
                    setTimeout(() => {
                        this.innerHTML = '<i class="fas fa-copy"></i>';
                    }, 2000);
                }
            });
        });
    }

    // Search Functionality
    function initSearch() {
        const searchInput = document.querySelector('.docs-search input');
        
        if (searchInput) {
            searchInput.addEventListener('input', function(e) {
                const query = e.target.value.toLowerCase();
                const sections = document.querySelectorAll('.docs-section');
                
                sections.forEach(section => {
                    const text = section.textContent.toLowerCase();
                    
                    if (query === '' || text.includes(query)) {
                        section.style.display = 'block';
                        
                        // Highlight matching text
                        if (query !== '') {
                            highlightText(section, query);
                        } else {
                            removeHighlight(section);
                        }
                    } else {
                        section.style.display = 'none';
                    }
                });
            });
        }
    }

    // Highlight search text
    function highlightText(element, query) {
        const walker = document.createTreeWalker(
            element,
            NodeFilter.SHOW_TEXT,
            null,
            false
        );
        
        let node;
        while (node = walker.nextNode()) {
            const parent = node.parentNode;
            
            if (parent.tagName !== 'SCRIPT' && parent.tagName !== 'STYLE' && parent.tagName !== 'CODE') {
                const text = node.textContent;
                const regex = new RegExp(`(${query})`, 'gi');
                
                if (regex.test(text)) {
                    const span = document.createElement('span');
                    span.innerHTML = text.replace(regex, '<mark>$1</mark>');
                    parent.replaceChild(span, node);
                }
            }
        }
    }

    // Remove highlight
    function removeHighlight(element) {
        const marks = element.querySelectorAll('mark');
        marks.forEach(mark => {
            const parent = mark.parentNode;
            parent.replaceChild(document.createTextNode(mark.textContent), mark);
            parent.normalize();
        });
    }

    // Table of Contents
    function initTableOfContents() {
        const tocContainer = document.querySelector('.docs-toc');
        
        if (tocContainer) {
            const headings = document.querySelectorAll('.docs-content h2, .docs-content h3');
            const tocList = document.createElement('ul');
            
            headings.forEach(heading => {
                const li = document.createElement('li');
                const a = document.createElement('a');
                
                // Generate ID if not present
                if (!heading.id) {
                    heading.id = heading.textContent.toLowerCase()
                        .replace(/[^\w\s]/g, '')
                        .replace(/\s+/g, '-');
                }
                
                a.href = '#' + heading.id;
                a.textContent = heading.textContent;
                
                if (heading.tagName === 'H3') {
                    li.style.paddingLeft = '20px';
                    a.style.fontSize = '0.875rem';
                }
                
                li.appendChild(a);
                tocList.appendChild(li);
                
                // Smooth scroll
                a.addEventListener('click', function(e) {
                    e.preventDefault();
                    heading.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                });
            });
            
            tocContainer.appendChild(tocList);
        }
    }

    // Version Selector
    function initVersionSelector() {
        const versionSelector = document.querySelector('.version-selector select');
        
        if (versionSelector) {
            versionSelector.addEventListener('change', function(e) {
                const version = e.target.value;
                // Redirect to version-specific docs
                window.location.href = `/docs/${version}/`;
            });
        }
    }

    // Mobile Sidebar Toggle
    function initMobileSidebar() {
        const sidebar = document.querySelector('.docs-sidebar');
        const content = document.querySelector('.docs-content');
        
        if (sidebar && window.innerWidth <= 768) {
            // Create toggle button
            const toggleBtn = document.createElement('button');
            toggleBtn.className = 'sidebar-toggle';
            toggleBtn.innerHTML = '<i class="fas fa-bars"></i>';
            toggleBtn.setAttribute('aria-label', 'Toggle sidebar');
            
            content.insertBefore(toggleBtn, content.firstChild);
            
            toggleBtn.addEventListener('click', function() {
                sidebar.classList.toggle('open');
                this.innerHTML = sidebar.classList.contains('open') 
                    ? '<i class="fas fa-times"></i>' 
                    : '<i class="fas fa-bars"></i>';
            });
            
            // Close sidebar when clicking outside
            document.addEventListener('click', function(e) {
                if (!sidebar.contains(e.target) && !toggleBtn.contains(e.target)) {
                    sidebar.classList.remove('open');
                    toggleBtn.innerHTML = '<i class="fas fa-bars"></i>';
                }
            });
        }
    }

    // API Reference Interactivity
    function initApiReference() {
        const methodHeaders = document.querySelectorAll('.docs-section h4');
        
        methodHeaders.forEach(header => {
            // Add expand/collapse for method details
            if (header.nextElementSibling && header.nextElementSibling.tagName === 'PRE') {
                header.style.cursor = 'pointer';
                header.innerHTML = '<i class="fas fa-chevron-right"></i> ' + header.innerHTML;
                
                const content = header.nextElementSibling;
                const description = content.nextElementSibling;
                
                // Initially collapsed
                content.style.display = 'none';
                if (description && description.tagName === 'P') {
                    description.style.display = 'none';
                }
                
                header.addEventListener('click', function() {
                    const isExpanded = content.style.display === 'block';
                    
                    if (isExpanded) {
                        content.style.display = 'none';
                        if (description && description.tagName === 'P') {
                            description.style.display = 'none';
                        }
                        this.querySelector('i').className = 'fas fa-chevron-right';
                    } else {
                        content.style.display = 'block';
                        if (description && description.tagName === 'P') {
                            description.style.display = 'block';
                        }
                        this.querySelector('i').className = 'fas fa-chevron-down';
                    }
                });
            }
        });
    }

    // Keyboard Navigation
    function initKeyboardNavigation() {
        document.addEventListener('keydown', function(e) {
            // Ctrl/Cmd + K for search
            if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
                e.preventDefault();
                const searchInput = document.querySelector('.docs-search input');
                if (searchInput) {
                    searchInput.focus();
                }
            }
            
            // Escape to close mobile sidebar
            if (e.key === 'Escape') {
                const sidebar = document.querySelector('.docs-sidebar');
                if (sidebar && sidebar.classList.contains('open')) {
                    sidebar.classList.remove('open');
                }
            }
        });
    }

    // Add copy button styles
    function addCopyButtonStyles() {
        const style = document.createElement('style');
        style.textContent = `
            .copy-code-btn {
                position: absolute;
                top: 10px;
                right: 10px;
                background: rgba(255, 255, 255, 0.1);
                border: 1px solid rgba(255, 255, 255, 0.2);
                border-radius: 4px;
                padding: 5px 10px;
                color: #fff;
                cursor: pointer;
                font-size: 14px;
                transition: all 0.3s ease;
                opacity: 0;
            }
            
            pre:hover .copy-code-btn {
                opacity: 1;
            }
            
            .copy-code-btn:hover {
                background: rgba(255, 255, 255, 0.2);
            }
            
            .copy-code-btn.copied {
                background: #00e599;
                border-color: #00e599;
            }
            
            .sidebar-toggle {
                display: none;
                position: fixed;
                top: 80px;
                left: 20px;
                z-index: 1000;
                background: var(--primary);
                color: white;
                border: none;
                border-radius: 50%;
                width: 50px;
                height: 50px;
                font-size: 20px;
                cursor: pointer;
                box-shadow: 0 4px 12px rgba(0, 229, 153, 0.3);
            }
            
            @media (max-width: 768px) {
                .sidebar-toggle {
                    display: flex;
                    align-items: center;
                    justify-content: center;
                }
                
                .docs-sidebar {
                    position: fixed;
                    left: -280px;
                    top: 60px;
                    height: calc(100vh - 60px);
                    z-index: 999;
                    transition: left 0.3s ease;
                    box-shadow: 2px 0 10px rgba(0, 0, 0, 0.1);
                }
                
                .docs-sidebar.open {
                    left: 0;
                }
            }
            
            mark {
                background: yellow;
                padding: 2px;
                border-radius: 2px;
            }
        `;
        document.head.appendChild(style);
    }

    // Initialize everything when DOM is ready
    document.addEventListener('DOMContentLoaded', function() {
        addCopyButtonStyles();
        initSidebar();
        initInstallationTabs();
        initCodeCopy();
        initSearch();
        initTableOfContents();
        initVersionSelector();
        initMobileSidebar();
        initApiReference();
        initKeyboardNavigation();
        
        // Initialize Prism.js if available
        if (typeof Prism !== 'undefined') {
            Prism.highlightAll();
        }
    });

})();