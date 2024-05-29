console.log("Javascript loaded")

var inputForm = document.getElementById("inputForm")

inputForm.addEventListener("submit", (e)=>{
  e.preventDefault()

  const formdata = new FormData(inputForm)

  fetch("handle", {
    method: "POST",
    body: formdata,
  }).then(
      response => response.text()
  ).then(
      (data) => {console.log(data);document.getElementById("serverMessageBox").innerHTML=data}
  ).catch(
      error => console.error(error)
  )
})
