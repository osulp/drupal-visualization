/* ========================================================================
Cole Crawford, 9/14/16
Used for Letter Locations Map
*/
	function set_linestrings_id(){
		//Run onload to set id for linestrings svg container to later toggle visibility
		//Set visibility of element
		document.getElementsByTagName("svg")[0].setAttribute("id", "linestrings_svg");
		document.getElementById("linestrings_svg").style.visibility = "visible";
	}
	
	function style_lines(){
	//Run only after set_linestrings_id()
		var all_lines = document.getElementById("linestrings_svg").children;
		for(var i = 0; i < all_lines.length; i++){
			if(all_lines[i].tagName == "g"){
				var paths = all_lines[i].children;
				paths[0].setAttribute("stroke-width","3");
			}
		}
	}
	
	function hide_lines(){
		var linestrings_svg = document.getElementById("linestrings_svg");
		if(linestrings_svg.style.visibility == "visible"){
			linestrings_svg.style.visibility = "hidden";
			document.getElementById("Show_Hide_Button").innerHTML = "Show Lines";
		} else {
			linestrings_svg.style.visibility = "visible";
			document.getElementById("Show_Hide_Button").innerHTML = "Hide Lines";
		}
	}