from SDS_Serverless_Data_Shield.cryptoHandler import CryptoHandler


class SecureContext:
    deleteTarget = []

    def __enter__(self):
        CryptoHandler.changeKey()

    def __exit__(self, exc_type, exc_val, exc_tb):
        for i in SecureContext.deleteTarget:
            i.clear()
            del i
        SecureContext.deleteTarget.clear()
        CryptoHandler.setZero()
