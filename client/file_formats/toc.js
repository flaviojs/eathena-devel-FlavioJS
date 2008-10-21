/*
 * Creates a TOC with links to the various sections.
 * Considers h2/h3/h4 headers to build it.
 * Generates the section numbers automatically and includes them in the headers and TOC links.
 *
 * Based on: "Dynamic Table of Contents script by Matt Whitlock <http://www.whitsoftdev.com/>"
 * Modified by FlavioJS
 */

function createLinkTo(node)
{
	/* create link */
	var a = document.createElement("a");
	a.setAttribute("href", "#" + node.id);
	/* duplicate contents */
	for( var i = 0; i < node.childNodes.length; ++i )
		a.appendChild(node.childNodes[i].cloneNode(true));
	return a;
}

function createListElement(type)
{
	var e = document.createElement(type);
	e.setAttribute("style","list-style-type: none;");
	return e;
}

function createToc(toc)
{
	var i2 = 0, i3 = 0, i4 = 0;
	/* remove contents */
	while( toc.hasChildNodes() )
		toc.removeChild(toc.lastChild);
	/* create toc */
	toc = toc.appendChild(createListElement("ul"));
	for( var i = 0; i < document.body.childNodes.length; ++i )
	{
		var node = document.body.childNodes[i];
		var tagName = node.nodeName.toLowerCase();
		if( tagName == "h4" )
		{
			++i4;
			if (i4 == 1) toc.lastChild.lastChild.lastChild.appendChild(createListElement("ul"));
			var section = i2 + "." + i3 + "." + i4;
			node.insertBefore(document.createTextNode(section + ". "), node.firstChild);
			node.id = "section" + section;
			toc.lastChild.lastChild.lastChild.lastChild.appendChild(document.createElement("li")).appendChild(createLinkTo(node));
		}
		else if( tagName == "h3" )
		{
			++i3, i4 = 0;
			if (i3 == 1) toc.lastChild.appendChild(createListElement("ul"));
			var section = i2 + "." + i3;
			node.insertBefore(document.createTextNode(section + ". "), node.firstChild);
			node.id = "section" + section;
			toc.lastChild.lastChild.appendChild(document.createElement("li")).appendChild(createLinkTo(node));
		}
		else if( tagName == "h2" )
		{
			++i2, i3 = 0, i4 = 0;
			var section = i2;
			node.insertBefore(document.createTextNode(section + ". "), node.firstChild);
			node.id = "section" + section;
			toc.appendChild(h2item = document.createElement("li")).appendChild(createLinkTo(node));
		}
	}
}
