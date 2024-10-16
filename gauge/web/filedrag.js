/*
filedrag.js - File Drag & Drop
*/
(function() {

	// getElementById
	function $id(id) {
		return document.getElementById(id);
	}

	// output information
	function Message(msg) {
		var m = $id("messages");
		m.innerHTML = msg + m.innerHTML;
	}

	// file drag hover
	function FileDragHover(e) {
		e.stopPropagation();
		e.preventDefault();
		e.target.className = (e.type == "dragover" ? "hover" : "");
	}

	// file selection
	function FileSelectHandler(e) {

		// cancel event and hover styling
		FileDragHover(e);

		// fetch FileList object
		var files = e.target.files || e.dataTransfer.files;
		
		if(files.length == 0) return;
		
		uploadFiles(files);
	}
	
	async function uploadFiles(files) {
		
		const formData = new FormData(); // Create a new FormData object.
		
		// Loop through each of the selected files.
		for (const file of files) {
			ParseFile(file); // display file information to output window
			formData.append('files[]', file, file.name); // Add the file to the request.
		}

		const settings = {
			method: 'POST',
			body: formData
		};
		
		try {
			const response = await fetch('/api/files/upload', settings);
			if (response.ok){
				
				getFileList(); // refresh the file list on the page
			} else {
				alert(response.status);
			}
		} catch(e) {
			alert(e.message);
		}  
	}

	// output file information
	function ParseFile(file) {

		Message(
			"<p>File info: <strong>" + file.name +
			"</strong> type: <strong>" + file.type +
			"</strong> size: <strong>" + file.size +
			"</strong> bytes</p>"
		);
	}

	// initialize
	function Init() {

		const fileselect = $id("fileselect"), filedrag = $id("filedrag");

		// file select
		// Set the value of the input to null on each onclick event. This will reset the input's value and trigger the onchange event even if the same path is selected.
		fileselect.onclick = function () { this.value = null; };
		fileselect.addEventListener("change", FileSelectHandler, false);

		// is XHR2 available?
		const xhr = new XMLHttpRequest();
		if (xhr.upload) {

			// file drop
			filedrag.addEventListener("dragover", FileDragHover, false);
			filedrag.addEventListener("dragleave", FileDragHover, false);
			filedrag.addEventListener("drop", FileSelectHandler, false);
			filedrag.style.display = "block";
		}

	}

	// call initialization file
	if (window.File && window.FileList && window.FileReader) {
		Init();
	}

})();