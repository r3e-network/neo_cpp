# Contributing to Neo N3 C++ Node

We love your input! We want to make contributing to the Neo N3 C++ Node as easy and transparent as possible, whether it's:

- Reporting a bug
- Discussing the current state of the code
- Submitting a fix
- Proposing new features
- Becoming a maintainer

## Development Process

We use GitHub to host code, to track issues and feature requests, as well as accept pull requests.

### Pull Requests

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Pull Request Process

1. Ensure any install or build dependencies are removed before the end of the layer when doing a build.
2. Update the README.md or documentation with details of changes to the interface, this includes new environment variables, exposed ports, useful file locations, and container parameters.
3. Increase the version numbers in any examples files and the README.md to the new version that this Pull Request would represent. The versioning scheme we use is [SemVer](http://semver.org/).
4. You may merge the Pull Request in once you have the sign-off of two other developers, or if you do not have permission to do that, you may request the second reviewer to merge it for you.

## Coding Style

We use the Google C++ Style Guide with some modifications:

- Use 4 spaces for indentation
- Use camelCase for function names
- Use PascalCase for class names
- Use snake_case for variable names
- Use UPPER_SNAKE_CASE for constants and macros

A `.clang-format` file is provided in the root directory to enforce the code style.

## Commit Message Convention

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification for commit messages:

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

Where `type` is one of:
- `feat`: A new feature
- `fix`: A bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or modifying tests
- `chore`: Changes to the build process or tools

## Branch Naming Convention

- `feature/feature-name`: For new features
- `bugfix/bug-name`: For bug fixes
- `refactor/refactor-name`: For code refactoring
- `docs/docs-name`: For documentation changes
- `test/test-name`: For test changes

## Testing

We use Google Test for unit testing. All new code should have corresponding unit tests. Significant changes should also include integration tests.

To run the tests:

```bash
cd build
cmake --build . --target test
```

## Documentation

We use Doxygen for API documentation. All public APIs should be documented using Doxygen-style comments.

To generate the documentation:

```bash
cd build
cmake --build . --target docs
```

## Reporting Bugs

We use GitHub issues to track public bugs. Report a bug by opening a new issue; it's that easy!

### Bug Report Template

```
## Bug Description

A clear and concise description of what the bug is.

## Steps to Reproduce

1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

## Expected Behavior

A clear and concise description of what you expected to happen.

## Actual Behavior

A clear and concise description of what actually happened.

## Screenshots

If applicable, add screenshots to help explain your problem.

## Environment

- OS: [e.g. Ubuntu 20.04]
- Compiler: [e.g. GCC 9.3.0]
- Version: [e.g. 0.1.0]

## Additional Context

Add any other context about the problem here.
```

## Feature Requests

We use GitHub issues to track feature requests. Request a feature by opening a new issue with the "feature request" template.

### Feature Request Template

```
## Feature Description

A clear and concise description of the feature you're requesting.

## Problem Statement

A clear and concise description of what the problem is. Ex. I'm always frustrated when [...]

## Proposed Solution

A clear and concise description of what you want to happen.

## Alternative Solutions

A clear and concise description of any alternative solutions or features you've considered.

## Additional Context

Add any other context or screenshots about the feature request here.
```

## License

By contributing, you agree that your contributions will be licensed under the project's MIT License.

## References

This document was adapted from the open-source contribution guidelines for [Facebook's Draft](https://github.com/facebook/draft-js/blob/master/CONTRIBUTING.md).
