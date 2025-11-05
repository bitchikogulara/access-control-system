const mongoose = require('mongoose')

const CardSchema = new mongoose.Schema({
  uid: {
    type: String,
    required: [true, 'must provide card UID'],
    trim: true,
    maxlength: [8, 'UID length must be 8 charackters'],
    minlength: [8, 'UID length must be 8 charackters'],
  },
  cardholderName: {
    type: String,
    required: [true, 'must provide cardholder Name'],
    trim: true,
  },
  active:{
    type: Boolean,
    default: true
  }
})

module.exports = mongoose.model('Card', CardSchema)
