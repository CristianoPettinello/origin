main.py:
    - Main file which calls function defined in Htt for processing image in folders.

HTT.py
    - Entry point for the processing functions.
    - Implements preprocessing functions (filtering, thresholding, etc.)
    - Model optimization is delegated to the OptimizationEngine class.

OptimizationEngine.py:
    - Setup of the optimization problem, setup of the model, precomputation of the loss field.
    - The actual optimization is performed by pso.py


pso.py:
    - General-purpose Particle Swarm Optimization
    - It is called

ScheimpflugModel.py, RX.py, RY.py, RZ.py
    - Scheimpflug deformation model for mapping points from the object plane to the camera sensor plane.

MiniBevelModel_S.py
    - Class implementing the Mini Bevel Model.
    - The model is defined in the object plane and converted into the image plane.

---------------------------------------------------

CustomBevelModel_S.py
    - Class implementing the Custom Bevel Model.
    - WARNING: This is an old definition and should not be used since it need to be refactored.

TBevelModel_S.py
    - Class implementing the T model
    - WARNING: This is an old definition and should not be used since it need to be refactored.

