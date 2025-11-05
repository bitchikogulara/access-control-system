const mongoose = require('mongoose')

const LogSchema = new mongoose.Schema({
    uid: {
        type: String,
        required: [true, 'must provide card UID'],
        trim: true,
        maxlength: [8, 'UID length must be 8 charackters'],
        minlength: [8, 'UID length must be 8 charackters'],
    },
    cardholderName: {
        type: String,
        default: "N/A",
        trim: true,
    },
    accessGranted: {
        type: Boolean,
        default: false
    },
    date: {
        type: Date,
        default: Date.now
    },
    mac: String,
    ip: String,
    rssi: Number
}
)

module.exports = mongoose.model('Log', LogSchema)
