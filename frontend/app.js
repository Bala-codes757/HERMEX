const API = "http://localhost:8080/api";

let buses = [];
let selectedBus = null;
let selectedSeats = [];
let passengers = [];
let currentBooking = null;

const pages = [
    "homePage",
    "resultsPage",
    "seatsPage",
    "passengerPage",
    "reviewPage",
    "confirmationPage",
    "bookingsPage"
];

function showPage(id) {
    pages.forEach(page => {
        document.getElementById(page).classList.remove("active");
    });

    document.getElementById(id).classList.add("active");

    window.scrollTo({
        top: 0,
        behavior: "smooth"
    });
}

function showHome() {
    showPage("homePage");
}

function showResults() {
    showPage("resultsPage");
}

function showSeats() {
    showPage("seatsPage");
}

function showPassengers() {
    showPage("passengerPage");
}

function showBookings() {
    loadBookings();
    showPage("bookingsPage");
}

function showToast(message) {
    const toast = document.getElementById("toast");

    toast.textContent = message;
    toast.classList.add("show");

    setTimeout(() => {
        toast.classList.remove("show");
    }, 2500);
}

async function searchBuses() {

    const from =
        document.getElementById("fromInput").value;

    const to =
        document.getElementById("toInput").value;

    const date =
        document.getElementById("dateInput").value;

    document.getElementById("resultFrom").textContent =
        from;

    document.getElementById("resultTo").textContent =
        to;

    document.getElementById("resultDate").textContent =
        date || "05 Oct 2026";

    try {

        const response =
            await fetch(`${API}/buses`);

        const data =
            await response.json();

        buses = data.buses || [];

        renderBuses();

        showPage("resultsPage");

    } catch (error) {

        showToast(
            "Unable to connect to HERMEX."
        );
    }
}

function renderBuses() {

    const container =
        document.getElementById("busList");

    document.getElementById("busCount").textContent =
        `${buses.length} buses`;

    container.innerHTML = "";

    buses.forEach(bus => {

        const card =
            document.createElement("div");

        card.className =
            "bus-card";

        card.innerHTML = `
            <div>

                <div class="bus-name">
                    <h3>${bus.name}</h3>

                    <span class="rating">
                        ★ ${bus.rating}
                    </span>
                </div>

                <div class="bus-times">

                    <div class="bus-time">
                        <strong>${bus.departure}</strong>
                        <span>${bus.from}</span>
                    </div>

                    <div>
                        <div class="bus-line"></div>
                        <span class="bus-duration">
                            ${bus.duration}
                        </span>
                    </div>

                    <div class="bus-time">
                        <strong>${bus.arrival}</strong>
                        <span>${bus.to}</span>
                    </div>

                </div>

                <div class="bus-features">
                    ${bus.type}
                    • ${bus.layout}
                    • ${bus.amenities.join(" • ")}
                </div>

            </div>

            <div class="bus-price">

                <div>
                    <span>Starting from</span>
                    <strong>₹${bus.fare}</strong>
                </div>

                <button
                    class="select-bus"
                    onclick="selectBus('${bus.id}')"
                >
                    Select
                </button>

            </div>
        `;

        container.appendChild(card);
    });
}

async function selectBus(id) {

    selectedBus =
        buses.find(bus => bus.id === id);

    if (!selectedBus)
        return;

    selectedSeats = [];

    document.getElementById("selectedBusName").textContent =
        selectedBus.name;

    document.getElementById("selectedBusInfo").textContent =
        `${selectedBus.type} • ${selectedBus.layout} • ${selectedBus.departure}`;

    document.getElementById("selectedBusRating").textContent =
        selectedBus.rating;

    document.getElementById("summaryFrom").textContent =
        selectedBus.from;

    document.getElementById("summaryTo").textContent =
        selectedBus.to;

    document.getElementById("summaryDate").textContent =
        document.getElementById("resultDate").textContent;

    await renderSeats();

    showPage("seatsPage");
}

async function renderSeats() {

    try {

        const response =
            await fetch(`${API}/seats`);

        const data =
            await response.json();

        const grid =
            document.getElementById("seatGrid");

        grid.innerHTML = "";

        data.seats.forEach(seat => {

            const button =
                document.createElement("button");

            button.className =
                `seat ${seat.status}`;

            if (selectedSeats.includes(seat.number)) {
                button.classList.remove("available");
                button.classList.add("selected");
            }

            button.innerHTML = `
                <span class="seat-number">
                    ${String(seat.number).padStart(2, "0")}
                </span>

                <span class="seat-symbol">
                    ${seat.status === "booked"
                        ? "×"
                        : selectedSeats.includes(seat.number)
                            ? "◆"
                            : "○"}
                </span>
            `;

            if (seat.status !== "booked") {
                button.onclick = () =>
                    toggleSeat(seat.number);
            }

            grid.appendChild(button);
        });

        updateSummary();

    } catch (error) {

        showToast(
            "Unable to load seats."
        );
    }
}

function toggleSeat(number) {

    if (selectedSeats.includes(number)) {

        selectedSeats =
            selectedSeats.filter(
                seat => seat !== number
            );

    } else {

        if (selectedSeats.length >= 6) {

            showToast(
                "Maximum 6 seats per booking."
            );

            return;
        }

        selectedSeats.push(number);
        selectedSeats.sort((a, b) => a - b);
    }

    renderSeats();
}

function updateSummary() {

    const list =
        document.getElementById(
            "selectedSeatsList"
        );

    if (selectedSeats.length === 0) {

        list.innerHTML =
            `<span class="empty-selection">
                Select your seats
            </span>`;

    } else {

        list.innerHTML =
            selectedSeats
                .map(
                    seat =>
                        `<span class="selected-chip">
                            Seat ${String(seat).padStart(2, "0")}
                        </span>`
                )
                .join("");
    }

    const fare =
        selectedSeats.length *
        (selectedBus?.fare || 450);

    const fee =
        selectedSeats.length > 0
            ? 20
            : 0;

    const total =
        fare + fee;

    document.getElementById("seatFare").textContent =
        `₹${fare}`;

    document.getElementById("convenienceFee").textContent =
        `₹${fee}`;

    document.getElementById("totalFare").textContent =
        `₹${total}`;

    const button =
        document.getElementById("continueBtn");

    if (selectedSeats.length > 0) {
        button.classList.remove("disabled");
    } else {
        button.classList.add("disabled");
    }
}

function continueToPassengers() {

    if (selectedSeats.length === 0) {

        showToast(
            "Please select at least one seat."
        );

        return;
    }

    renderPassengerForms();

    showPage("passengerPage");
}

function renderPassengerForms() {

    const container =
        document.getElementById(
            "passengerForms"
        );

    container.innerHTML = "";

    selectedSeats.forEach((seat, index) => {

        const card =
            document.createElement("div");

        card.className =
            "passenger-card";

        card.innerHTML = `
            <div class="passenger-card-header">

                <strong>
                    PASSENGER ${index + 1}
                </strong>

                <span class="passenger-seat">
                    SEAT ${String(seat).padStart(2, "0")}
                </span>

            </div>

            <div class="form-grid">

                <div class="form-field">

                    <label>FULL NAME</label>

                    <input
                        type="text"
                        id="name-${seat}"
                        placeholder="Enter passenger name"
                    >

                </div>

                <div class="form-field">

                    <label>AGE</label>

                    <input
                        type="number"
                        id="age-${seat}"
                        min="1"
                        max="100"
                        placeholder="Age"
                    >

                </div>

                <div class="form-field">

                    <label>GENDER</label>

                    <select id="gender-${seat}">

                        <option value="">
                            Select
                        </option>

                        <option value="Male">
                            Male
                        </option>

                        <option value="Female">
                            Female
                        </option>

                        <option value="Other">
                            Other
                        </option>

                    </select>

                </div>

            </div>
        `;

        container.appendChild(card);
    });
}

function collectPassengers() {

    passengers = [];

    for (const seat of selectedSeats) {

        const name =
            document.getElementById(
                `name-${seat}`
            ).value.trim();

        const age =
            parseInt(
                document.getElementById(
                    `age-${seat}`
                ).value
            );

        const gender =
            document.getElementById(
                `gender-${seat}`
            ).value;

        if (!name || !age || !gender) {

            showToast(
                `Complete details for seat ${seat}.`
            );

            return false;
        }

        passengers.push({
            name,
            age,
            gender,
            seat
        });
    }

    return true;
}

function continueToReview() {

    if (!collectPassengers())
        return;

    renderReview();

    showPage("reviewPage");
}

function renderReview() {

    document.getElementById("reviewFrom").textContent =
        selectedBus.from;

    document.getElementById("reviewTo").textContent =
        selectedBus.to;

    document.getElementById("reviewDeparture").textContent =
        selectedBus.departure;

    document.getElementById("reviewArrival").textContent =
        selectedBus.arrival;

    document.getElementById("reviewBus").textContent =
        selectedBus.name;

    document.getElementById("reviewBusType").textContent =
        `${selectedBus.type} • ${selectedBus.layout}`;

    const container =
        document.getElementById(
            "reviewPassengers"
        );

    container.innerHTML = "";

    passengers.forEach(passenger => {

        const row =
            document.createElement("div");

        row.className =
            "review-passenger";

        row.innerHTML = `
            <span>
                ${passenger.name}
                <small>
                    ${passenger.gender} • ${passenger.age}
                </small>
            </span>

            <strong>
                Seat ${String(passenger.seat).padStart(2, "0")}
            </strong>
        `;

        container.appendChild(row);
    });

    const fare =
        selectedSeats.length *
        selectedBus.fare;

    const total =
        fare + 20;

    document.getElementById("reviewFare").textContent =
        `₹${fare}`;

    document.getElementById("reviewTotal").textContent =
        `₹${total}`;
}

async function confirmBooking() {

    const payload = {

        seats: selectedSeats,

        passengers: passengers
    };

    try {

        showToast(
            "Confirming your booking..."
        );

        const response =
            await fetch(
                `${API}/book`,
                {
                    method: "POST",

                    headers: {
                        "Content-Type":
                            "application/json"
                    },

                    body:
                        JSON.stringify(payload)
                }
            );

        const data =
            await response.json();

        if (!data.success) {

            showToast(
                data.message ||
                "Booking failed."
            );

            await renderSeats();

            return;
        }

        currentBooking =
            data;

        renderConfirmation();

        showPage("confirmationPage");

    } catch (error) {

        showToast(
            "Unable to connect to HERMEX."
        );
    }
}

function renderConfirmation() {

    document.getElementById(
        "confirmationId"
    ).textContent =
        currentBooking.bookingId;

    document.getElementById(
        "confirmationDate"
    ).textContent =
        currentBooking.date;

    document.getElementById(
        "confirmationSeats"
    ).textContent =
        currentBooking.seats
            ? selectedSeats.join(", ")
            : selectedSeats.join(", ");

    document.getElementById(
        "confirmationTotal"
    ).textContent =
        `₹${currentBooking.total}`;

    const container =
        document.getElementById(
            "confirmationPassengers"
        );

    container.innerHTML = "";

    currentBooking.passengers.forEach(
        passenger => {

            const row =
                document.createElement("div");

            row.className =
                "ticket-passenger";

            row.innerHTML = `
                <span>
                    ${passenger.name}
                </span>

                <strong>
                    ${String(passenger.seat)
                        .padStart(2, "0")}
                </strong>
            `;

            container.appendChild(row);
        }
    );
}

function downloadTicket() {

    if (!currentBooking)
        return;

    const ticket = `
============================================================
                         HERMEX
                        E-TICKET
============================================================

BOOKING ID     : ${currentBooking.bookingId}

FROM           : Chennai
TO             : Bengaluru
DATE           : ${currentBooking.date}
DEPARTURE      : ${currentBooking.departure}

BUS            : CITYLINE EXPRESS 204
TYPE           : AC SEATER
LAYOUT         : 2 + 2

------------------------------------------------------------
PASSENGER DETAILS
------------------------------------------------------------

${currentBooking.passengers.map(
    p =>
`SEAT ${String(p.seat).padStart(2, "0")}
NAME           : ${p.name}
AGE            : ${p.age}
GENDER         : ${p.gender}
`
).join("\n")}

------------------------------------------------------------

NUMBER OF SEATS : ${currentBooking.seats}
TICKET FARE     : ₹${currentBooking.fare}
CONVENIENCE FEE : ₹${currentBooking.fee}
TOTAL           : ₹${currentBooking.total}

------------------------------------------------------------
                  BOOKING CONFIRMED
------------------------------------------------------------

Thank you for travelling with HERMEX.

============================================================
`;

    const blob =
        new Blob(
            [ticket],
            { type: "text/plain" }
        );

    const url =
        URL.createObjectURL(blob);

    const link =
        document.createElement("a");

    link.href = url;

    link.download =
        `HERMEX_${currentBooking.bookingId}.txt`;

    link.click();

    URL.revokeObjectURL(url);
}

async function loadBookings() {

    const container =
        document.getElementById(
            "bookingsList"
        );

    try {

        const response =
            await fetch(
                `${API}/bookings`
            );

        const data =
            await response.json();

        if (
            !data.bookings ||
            data.bookings.length === 0
        ) {

            container.innerHTML = `
                <div class="feature">
                    <h3>No trips yet</h3>
                    <p>
                        Your HERMEX journeys will appear here.
                    </p>
                </div>
            `;

            return;
        }

        container.innerHTML = "";

        data.bookings.forEach(
            booking => {

                const item =
                    document.createElement("div");

                item.className =
                    "booking-item";

                item.innerHTML = `
                    <div class="booking-item-top">

                        <span class="booking-id">
                            ${booking.bookingId}
                        </span>

                        <strong>
                            ₹${booking.total}
                        </strong>

                    </div>

                    <div class="booking-route">
                        ${booking.from}
                        →
                        ${booking.to}
                    </div>

                    <div class="booking-meta">
                        ${booking.date}
                        •
                        ${booking.departure}
                        •
                        Seats ${booking.seats.join(", ")}
                    </div>
                `;

                container.appendChild(item);
            }
        );

    } catch (error) {

        container.innerHTML = `
            <div class="feature">
                <h3>Unable to load trips</h3>
                <p>
                    Please make sure the HERMEX backend is running.
                </p>
            </div>
        `;
    }
}

window.addEventListener(
    "DOMContentLoaded",
    () => {

        const date =
            document.getElementById(
                "dateInput"
            );

        const today =
            new Date();

        date.value =
            today.toISOString()
                .split("T")[0];
    }
);