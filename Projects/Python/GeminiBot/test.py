import json

text = {
    'candidates': [{
        'content': {
            'parts': [{
                'text': 'Hello! How can I help you today?\n'
            }], 
            'role': 'model'
        }, 'finishReason': 'STOP', 'avgLogprobs': -0.07186999320983886
    }], 
    'usageMetadata': {
        'promptTokenCount': 1, 'candidatesTokenCount': 10, 'totalTokenCount': 11, 'promptTokensDetails': [{
            'modality': 'TEXT', 'tokenCount': 1
        }], 'candidatesTokensDetails': [{
            'modality': 'TEXT', 'tokenCount': 10
        }]}, 'modelVersion': 'gemini-2.0-flash'
}

