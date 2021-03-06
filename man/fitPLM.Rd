\name{fitPLM}
\alias{fitPLM}
\title{Fit a Probe Level Model to Affymetrix Genechip Data.}
\description{
  This function converts an
  \code{\link[affy:AffyBatch-class]{AffyBatch}} into an 
  \code{\link{PLMset}} by fitting a specified robust linear model to the
  probe level data.
}
\usage{
fitPLM(object,model=PM ~ -1 + probes +samples,
       variable.type=c(default="factor"),
       constraint.type=c(default="contr.treatment"),
       subset=NULL,
       background=TRUE, normalize=TRUE, background.method="RMA.2",
       normalize.method="quantile", background.param=list(),
       normalize.param=list(), output.param=verify.output.param(),
       model.param=verify.model.param(object, model),
       verbosity.level=0)
}

\arguments{
  \item{object}{an \code{\link[affy:AffyBatch-class]{AffyBatch}}}
  \item{model}{A formula describing the model to fit. This is slightly
    different from the standard method of specifying formulae in R. Read
    the description below}
  \item{variable.type}{a way to specify whether variables in the model
    are factors or standard variables}
  \item{constraint.type}{should factor variables sum to zero or have
    first variable set to zero (endpoint constraint)}
  \item{subset}{a vector with the names of probesets to be used. If NULL
  then all probesets are used.}
  \item{normalize}{logical value. If \code{TRUE} normalize data using
    quantile normalization}
  \item{background}{logical value. If \code{TRUE} background correct
    using RMA background correction}
  \item{background.method}{name of background method to use.}
  \item{normalize.method}{name of normalization method to use.}
  \item{background.param}{A list of parameters for background routines}
  \item{normalize.param}{A list of parameters for normalization
    routines}
  \item{output.param}{A list of parameters controlling optional output
    from the routine.}
  \item{model.param}{A list of parameters controlling model procedure}
  \item{verbosity.level}{An integer specifying how much to print
    out. Higher values indicate more verbose. A value of 0 will print nothing}
}
\details{
  This function fits robust Probe Level linear Models to all the probesets in
  an \code{\link[affy:AffyBatch-class]{AffyBatch}}. This is carried out
  on a probeset by probeset basis. The user has quite a lot of control
  over which model is used and what outputs are stored. For more details
  please read the vignette. 
}
\value{
  An \code{\link{PLMset}}
}

\author{Ben Bolstad \email{bmb@bmbolstad.com}}

\references{Bolstad, BM (2004) \emph{Low Level Analysis of High-density
    Oligonucleotide Array Data: Background, Normalization and
    Summarization}. PhD Dissertation. University of California,
  Berkeley.}

\seealso{\code{\link[affy]{expresso}},
  \code{\link[affy]{rma}}, \code{\link{threestep}}}

\examples{
if (require(affydata)) {
  data(Dilution)
  Pset <- fitPLM(Dilution, model=PM ~ -1 + probes + samples)
  se(Pset)[1:5,]

  image(Pset)
  NUSE(Pset) 

  #now lets try a wider class of models
  \dontrun{Pset <- fitPLM(Dilution,model=PM ~ -1 + probes +liver,
  normalize=FALSE,background=FALSE)} 
  \dontrun{coefs(Pset)[1:10,]}

  \dontrun{Pset <- fitPLM(Dilution,model=PM ~ -1 + probes + liver +
  scanner, normalize=FALSE,background=FALSE)}
  coefs(Pset)[1:10,]

  #try liver as a covariate
  logliver <- log2(c(20,20,10,10))
  \dontrun{Pset <- fitPLM(Dilution, model=PM~-1+probes+logliver+scanner,
  normalize=FALSE, background=FALSE, variable.type=c(logliver="covariate"))} 
  coefs(Pset)[1:10,]

  #try a different se.type
  \dontrun{Pset <- fitPLM(Dilution, model=PM~-1+probes+scanner,
  normalize=FALSE,background=FALSE,m odel.param=list(se.type=2))} 
  se(Pset)[1:10,]
}
}

\keyword{manip}

