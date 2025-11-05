const express = require('express')
const router = express.Router()

const {
    validateCard,
    addCard,
    getAllCards,
    deleteCard,
    updateCard
} = require('../controller/card-auth.js')

router.route('/').get(getAllCards).post(addCard)
router.route('/:id').delete(deleteCard).patch(updateCard)
router.route('/validate').post(validateCard)

module.exports = router;