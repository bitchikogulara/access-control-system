const Card = require('../model/Card.js')
const Log = require('../model/Log.js')
const asyncWrapper = require('../middleware/async')
const { createCustomError } = require('../errors/custom-error')

const getAllCards = asyncWrapper(async (req,res)=>{
    const cards = await Card.find({});
    res.status(200).json({cards})
})

const validateCard = asyncWrapper(async (req, res) => {
    const uid = req.body.card_uid;
    console.log(uid);

    const card = await Card.findOne({uid:uid})
    const {_id: LogID} = await Log.create({
            uid: uid,
            mac: req.body.mac,
            ip: req.body.ip,
            rssi: req.body.rssi
        })
    if(!!card){
        await Log.findOneAndUpdate({_id: LogID}, {
            cardholderName: card.cardholderName
        })
    }

    if(!card || !card.active){
        await res.status(200).json({ allow: false, uid });
    }else{
        await Log.findOneAndUpdate({_id: LogID}, {
            accessGranted: true
        })
        res.status(200).json({ allow: true, card });
    }
})

const addCard = asyncWrapper(async (req, res) => {
    const card = await Card.create(req.body);
    res.status(201).json({card})
})

const deleteCard = asyncWrapper(async (req, res, next) => {
  const { id: CardID } = req.params;
  const card = await Card.findOneAndDelete({ _id: CardID });

  if (!card) {
    return next(createCustomError(`Can't find Card with ID: ${CardID}`, 404));
  }

  res.status(200).json({ card });
})

const updateCard = asyncWrapper(async(req, res,next)=>{
    const{id: CardID} = req.params
    
    const card = await Card.findOneAndUpdate({_id: CardID}, req.body, {
        new: true,
        runValidators: true
    })

    if (!card) {
        return next(createCustomError(`Can't find Card with ID: ${CardID}`, 404));
    }

    res.status(200).json({ card });
})

const getAllLogs = asyncWrapper(async (req,res)=>{
    const logs = await Log.find({});
    res.status(200).json({logs})
})


module.exports = {
    validateCard,
    addCard,
    getAllCards,
    deleteCard,
    updateCard,
    getAllLogs
}