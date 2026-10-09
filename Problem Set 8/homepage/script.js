document.addEventListener("DOMContentLoaded", function () {
  // Home: greeting based on the visitor's local time
  const greeting = document.getElementById("greeting");
  if (greeting) {
    const hour = new Date().getHours();
    const part = hour < 12 ? "morning" : hour < 18 ? "afternoon" : "evening";
    greeting.textContent = "Good " + part + ", and welcome!";
  }

  // Home: show a random baking tip on button click
  const tips = [
    "Use warm water to wake up a sleepy starter.",
    "Score the loaf just before baking so it can expand.",
    "Let bread cool for an hour before slicing.",
    "A hotter oven gives a crunchier crust."
  ];
  const tipButton = document.getElementById("tip-button");
  if (tipButton) {
    tipButton.addEventListener("click", function () {
      document.getElementById("tip").textContent =
        tips[Math.floor(Math.random() * tips.length)];
    });
  }

  // Sourdough: scale the flour amount by number of loaves
  const loaves = document.getElementById("loaves");
  if (loaves) {
    loaves.addEventListener("input", function () {
      const count = Math.max(1, Number(loaves.value) || 1);
      document.getElementById("flour").textContent = 500 * count;
    });
  }

  // Contact: validate the form and show a confirmation
  const form = document.getElementById("contact-form");
  if (form) {
    form.addEventListener("submit", function (event) {
      event.preventDefault();
      const name = document.getElementById("name").value.trim();
      const message = document.getElementById("message").value.trim();
      const result = document.getElementById("form-result");
      result.classList.remove("d-none", "alert-success", "alert-danger");
      if (name === "" || message === "") {
        result.classList.add("alert-danger");
        result.textContent = "Please fill in both your name and a message.";
      } else {
        result.classList.add("alert-success");
        result.textContent = "Thanks, " + name + "! Your message was received.";
        form.reset();
      }
    });
  }
});

