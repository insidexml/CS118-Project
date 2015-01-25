var spinning = false 
function spin() {
    var leekspin = document.getElementById("leekspin");
    if (spinning) {
      leekspin.pause()
    }
    else {
      leekspin.play()
    }
    spinning = !spinning
}
