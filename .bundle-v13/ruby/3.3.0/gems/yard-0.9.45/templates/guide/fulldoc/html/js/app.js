window.generateTOC = () => {
	const fileContents = document.getElementById("filecontents");
	const tocRoot = document.getElementById("toc");
	const topLevel = document.createElement("ol");
	let currentList = topLevel;
	let lastLevel = 1;
	let currentItem = null;
	let counter = 0;
	let hasEntries = false;

	if (!fileContents || !tocRoot) return;

	topLevel.className = "top";
	const headings = fileContents.querySelectorAll(
		":scope > h1, :scope > h2, :scope > h3, :scope > h4, :scope > h5, :scope > h6",
	);

	Array.prototype.forEach.call(headings, (heading) => {
		let level;

		if (heading.id === "filecontents") return;
		hasEntries = true;
		level = parseInt(heading.tagName.substring(1), 10);

		if (!heading.id) {
			let proposedId = heading.textContent.replace(/[^a-z0-9-]/gi, "_");
			if (document.getElementById(proposedId)) proposedId += counter++;
			heading.id = proposedId;
		}

		if (level > lastLevel) {
			while (level > lastLevel) {
				if (!currentItem) {
					currentItem = document.createElement("li");
					currentList.appendChild(currentItem);
				}
				const nestedList = document.createElement("ol");
				currentItem.appendChild(nestedList);
				currentList = nestedList;
				currentItem = null;
				lastLevel += 1;
			}
		} else if (level < lastLevel) {
			while (level < lastLevel && currentList.parentElement) {
				currentList = currentList.parentElement.parentElement;
				lastLevel -= 1;
			}
		}

		const item = document.createElement("li");
		const link = document.createElement("a");
		link.href = `#${heading.id}`;
		link.textContent = heading.textContent;
		item.appendChild(link);
		currentList.appendChild(item);
		currentItem = item;
	});

	if (!hasEntries) return;
	tocRoot.appendChild(topLevel);
};
