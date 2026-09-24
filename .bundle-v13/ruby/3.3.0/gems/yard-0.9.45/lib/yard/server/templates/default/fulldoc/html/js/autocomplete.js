(() => {
	function query(selector, root) {
		return (root || document).querySelector(selector);
	}

	function ready(callback) {
		if (document.readyState === "loading") {
			document.addEventListener("DOMContentLoaded", callback, { once: true });
		} else {
			callback();
		}
	}

	function createAutocomplete(input) {
		const form = input.form;
		const results = document.createElement("div");
		const list = document.createElement("ul");
		let requestTimer = null;
		let controller = null;
		let items = [];
		let activeIndex = -1;
		let blurTimer = null;

		if (!form) return;

		results.className = "ac_results";
		results.hidden = true;
		results.setAttribute("role", "listbox");
		results.id = `${input.id}_results`;
		list.setAttribute("role", "presentation");
		results.appendChild(list);
		input.setAttribute("autocomplete", "off");
		input.setAttribute("aria-autocomplete", "list");
		input.setAttribute("aria-controls", results.id);
		input.setAttribute("aria-expanded", "false");
		form.appendChild(results);

		function syncResultsWidth() {
			results.style.width = `${input.offsetWidth}px`;
		}

		function hideResults() {
			results.hidden = true;
			input.setAttribute("aria-expanded", "false");
			input.removeAttribute("aria-activedescendant");
			activeIndex = -1;
			items = [];
			list.innerHTML = "";
		}

		function setActive(index) {
			if (!items.length) return;
			activeIndex = (index + items.length) % items.length;
			items.forEach((item, itemIndex) => {
				item.element.classList.toggle("ac_over", itemIndex === activeIndex);
			});
			input.setAttribute(
				"aria-activedescendant",
				items[activeIndex].element.id,
			);
		}

		function selectItem(item) {
			input.value = item.values[1];
			window.location.href = item.values[3];
		}

		function renderItems(lines) {
			syncResultsWidth();
			list.innerHTML = "";
			items = lines.map((line, index) => {
				const values = line.split(",");
				const element = document.createElement("li");
				const label = document.createElement("span");
				const namespace = document.createElement("small");

				element.id = `${results.id}_item_${index}`;
				element.setAttribute("role", "option");
				element.className = index % 2 === 0 ? "ac_even" : "ac_odd";
				label.textContent = values[0];
				element.appendChild(label);

				if (values[1] !== "") {
					namespace.textContent = `(${values[1]})`;
					element.appendChild(document.createTextNode(" "));
					element.appendChild(namespace);
				}

				element.addEventListener("mouseenter", () => {
					setActive(index);
				});
				element.addEventListener("mousedown", (event) => {
					event.preventDefault();
					selectItem(items[index]);
				});

				list.appendChild(element);

				return { element: element, values: values };
			});

			if (items.length) {
				results.hidden = false;
				input.setAttribute("aria-expanded", "true");
				setActive(0);
			} else {
				hideResults();
			}
		}

		function fetchResults(term) {
			if (controller) controller.abort();
			controller = new AbortController();
			input.classList.add("ac_loading");

			fetch(`${form.action}?q=${encodeURIComponent(term)}&_=${Date.now()}`, {
				headers: {
					"X-Requested-With": "XMLHttpRequest",
				},
				signal: controller.signal,
			})
				.then((response) => response.text())
				.then((text) => {
					const lines = text
						.split("\n")
						.map((line) => line.trim())
						.filter(Boolean);

					renderItems(lines);
				})
				.catch((error) => {
					if (error.name !== "AbortError") hideResults();
				})
				.finally(() => {
					input.classList.remove("ac_loading");
				});
		}

		input.addEventListener("input", () => {
			clearTimeout(requestTimer);
			if (blurTimer) clearTimeout(blurTimer);

			if (!input.value.trim()) {
				hideResults();
				return;
			}

			requestTimer = setTimeout(() => {
				fetchResults(input.value.trim());
			}, 200);
		});

		input.addEventListener("keydown", (event) => {
			if (
				results.hidden &&
				(event.key === "ArrowDown" || event.key === "ArrowUp")
			) {
				if (!input.value.trim()) return;
				fetchResults(input.value.trim());
				return;
			}

			if (event.key === "ArrowDown") {
				event.preventDefault();
				setActive(activeIndex + 1);
			} else if (event.key === "ArrowUp") {
				event.preventDefault();
				setActive(activeIndex - 1);
			} else if (event.key === "Enter") {
				if (activeIndex >= 0 && items[activeIndex]) {
					event.preventDefault();
					selectItem(items[activeIndex]);
				}
			} else if (event.key === "Escape") {
				hideResults();
			}
		});

		input.addEventListener("blur", () => {
			blurTimer = setTimeout(hideResults, 150);
		});

		input.addEventListener("focus", () => {
			syncResultsWidth();
			if (items.length) {
				results.hidden = false;
				input.setAttribute("aria-expanded", "true");
			}
		});

		document.addEventListener("click", (event) => {
			if (!form.contains(event.target)) hideResults();
		});

		window.addEventListener("resize", syncResultsWidth);
		syncResultsWidth();
	}

	ready(() => {
		const input = query("#search_box");
		if (input) createAutocomplete(input);
	});
})();
