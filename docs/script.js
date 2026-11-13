window.addEventListener('load', () => {
  document.querySelector('.header .summary')?.remove()

  const brief = document.querySelector('.contents p')

  if (brief?.querySelector('a:last-child')?.innerText === 'More...') {
    brief.remove()
  }

  const headerDetails = document.querySelector('.contents #header-details')

  if (headerDetails) {
    const contents = document.querySelector('.contents')
    const textBlock = contents.querySelector('#header-details + .textblock')

    contents.prepend(contents.removeChild(textBlock))
    headerDetails.remove()
  }

  const redundantLinks = [...document.querySelectorAll('td.memItemRight .el')].filter(el => el.innerText.endsWith('callback') && el.previousSibling?.nodeValue?.trimStart()?.endsWith(`&${el.innerText}`))

  for (const redundantLink of redundantLinks) {
    redundantLink.remove()
  }

  const doubleLinks = [...document.querySelectorAll('td.memItemRight .el')].filter(el => el.innerText.endsWith('callback') && el.previousSibling?.nodeValue?.trimStart()?.endsWith(` ${el.innerText}`))

  for (const doubleLink of doubleLinks) {
    doubleLink.previousSibling.nodeValue = doubleLink.previousSibling.nodeValue.replace(/\w*callback$/, '')
  }
})