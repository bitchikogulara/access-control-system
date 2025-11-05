const express = require('express')
const router = express.Router()

const {
    getAllLogs
} = require('../controller/card-auth.js')

router.route('/').get(getAllLogs)

module.exports = router