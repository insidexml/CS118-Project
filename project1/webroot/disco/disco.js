var partying = false
var counter = 0
var colors = ["red", "blue", "green", "pink", "orange"]

window.setInterval(function () {
    counter += 1
    if (partying) {
        document.body.style.background = colors[counter % 5]
    }
}, 500)

function party() {
    partying = !partying
    if (!partying) {
        document.body.style.background = "white"
    }
}
